import express from 'express';
import Ticker from './models/Ticker.js';
import { getQuote, getHistory } from './provider/yahoo.js';

const app = express();

const QUOTE_TTL = 15; // minutes

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

app.get('/', (req, res) => {
  res.json({ status: 'ok', message: 'Pebble Stocks API' });
});

export default app;
