/*
 * Pebble Stocks — API routes and the MongoDB-backed quote cache.
 * Copyright (C) 2026 Claudio Rojas
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License, version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

import express from 'express';
import mongoose from 'mongoose';
import Ticker from './models/Ticker.js';
import { getQuote, getHistory } from './provider/yahoo.js';

const app = express();

//
//    MONGODB Connection
//
///////////////////////////////////////////////////////

let _conn = null;
async function connectDB() {
  if (_conn) return;
  _conn = await mongoose.connect(process.env.MONGODB_URI, {
    serverSelectionTimeoutMS: 5000,
    connectTimeoutMS: 10000,
  });
}

app.use(async (req, res, next) => {
  try {
    await connectDB();
    next();
  } catch (err) {
    console.error('MongoDB connection failed:', err.message);
    res.status(503).json({ error: 'Database unavailable' });
  }
});

//
//    API Helpers
//
///////////////////////////////////////////////////////

const QUOTE_TTL = 5; // minutes — matches cron refresh interval

const HISTORY_TTL = {
  '1D':  5,
  '1W':  30,
  '1M':  1440,
  '3M':  1440,
  'YTD': 1440,
  '1Y':  1440,
};

function isFresh(updatedAt, ttlMinutes) {
  if (!updatedAt) return false;
  const age = (Date.now() - new Date(updatedAt).getTime()) / 1000 / 60;
  return age < ttlMinutes;
}

//
//    API Routes
//
///////////////////////////////////////////////////////

// GET /api/tickers/:ticker — current quote
app.get('/api/tickers/:ticker', async (req, res) => {
  const ticker = req.params.ticker.toUpperCase();

  try {
    let doc = await Ticker.findOne({ ticker });

    if (doc?.quote && isFresh(doc.quote.updatedAt, QUOTE_TTL)) {
      return res.json({
        ticker,
        price: doc.quote.price,
        change: doc.quote.change,
        marketHours: doc.quote.marketHours,
        changePercent: doc.quote.changePercent,
      });
    }

    // Cache miss or stale — fetch from Yahoo Finance
    const quote = await getQuote(ticker);
    if (!quote) {
      return res.status(404).json({ error: 'Ticker not found' });
    }

    doc = await Ticker.findOneAndUpdate(
      { ticker },
      { quote },
      { upsert: true, returnDocument: 'after' }
    );

    res.json({
      ticker,
      price: doc.quote.price,
      marketHours: doc.quote.marketHours,
      change: doc.quote.change,
      changePercent: doc.quote.changePercent,
    });
  } catch (err) {
    console.error(`Error fetching quote for ${ticker}:`, err.message);
    res.status(500).json({ error: 'Failed to fetch quote' });
  }
});

// GET /api/tickers/:ticker/history?range=1M — historical prices for graph
app.get('/api/tickers/:ticker/history', async (req, res) => {
  const ticker = req.params.ticker.toUpperCase();
  const range = (req.query.range || '1M').toUpperCase();

  if (!HISTORY_TTL[range]) {
    return res.status(400).json({ error: 'Invalid range', valid: Object.keys(HISTORY_TTL) });
  }

  try {
    const doc = await Ticker.findOne({ ticker });
    const cached = doc?.history?.get(range);

    if (cached?.data?.length && isFresh(cached.updatedAt, HISTORY_TTL[range])) {
      return res.json({ ticker, range, data: cached.data });
    }

    const data = await getHistory(ticker, range);

    if (!data) {
      return res.status(404).json({ error: 'No data available for this ticker/range' });
    }

    // Respond immediately — don't let a cache write failure block the watch
    res.json({ ticker, range, data });

    // Cache in MongoDB (fire and forget)
    Ticker.findOneAndUpdate(
      { ticker },
      { $set: { [`history.${range}`]: { data, updatedAt: new Date() } } },
      { upsert: true }
    ).catch(err => console.error(`Cache write failed for ${ticker}/${range}:`, err.message));
  } catch (err) {
    console.error(`Error fetching history for ${ticker} (${range}):`, err.message);
    res.status(500).json({ error: 'Failed to fetch history' });
  }
});

// Returns true during regular market hours (9:30–16:00 ET, Mon–Fri)
function isMarketOpen() {
  const et  = new Date(new Date().toLocaleString('en-US', { timeZone: 'America/New_York' }));
  const day = et.getDay();
  if (day === 0 || day === 6) return false;
  const minutes = et.getHours() * 60 + et.getMinutes();
  return minutes >= 570 && minutes < 960; // 9:30–16:00
}

// POST /api/refresh — refreshes all cached quotes, called by cron
app.post('/api/refresh', async (req, res) => {
  const secret = process.env.CRON_SECRET;
  if (secret && req.headers['x-cron-secret'] !== secret) {
    return res.status(401).json({ error: 'Unauthorized' });
  }

  if (!isMarketOpen()) {
    console.log('[refresh] Market closed — skipping');
    return res.json({ skipped: true, reason: 'market closed' });
  }

  const tickers = await Ticker.find({}).select('ticker').lean();
  if (!tickers.length) return res.json({ refreshed: 0 });

  const results = await Promise.allSettled(
    tickers.map(async ({ ticker }) => {
      const quote = await getQuote(ticker);
      if (quote) await Ticker.findOneAndUpdate({ ticker }, { quote });
      return ticker;
    })
  );

  const ok  = results.filter(r => r.status === 'fulfilled').map(r => r.value);
  const err = results.filter(r => r.status === 'rejected').map(r => r.reason?.message);
  console.log(`[refresh] ${ok.length} ok, ${err.length} failed`);
  res.json({ refreshed: ok.length, failed: err.length });
});

app.get('/', (req, res) => {
  res.json({ status: 'ok', message: 'Pebble Stocks API' });
});

export default app;
