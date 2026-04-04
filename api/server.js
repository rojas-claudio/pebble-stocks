import express from 'express';
import Ticker from './models/Ticker.js';
import { getQuote, getCandles } from './services/finnhub.js';

const app = express();

// Range config: Yahoo Finance range/interval params + cache TTL in minutes
const RANGES = {
  '1D':  { range: '1d',  interval: '5m',  ttl: 5 },
  '1W':  { range: '5d',  interval: '30m', ttl: 30 },
  '1M':  { range: '1mo', interval: '1d',  ttl: 1440 },
  '3M':  { range: '3mo', interval: '1d',  ttl: 1440 },
  'YTD': { range: 'ytd', interval: '1d',  ttl: 1440 },
  '1Y':  { range: '1y',  interval: '1d',  ttl: 1440 },
};

const QUOTE_TTL = 15; // minutes

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

    // Cache miss or stale — fetch from Finnhub
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

  if (!RANGES[range]) {
    return res.status(400).json({
      error: 'Invalid range',
      valid: Object.keys(RANGES),
    });
  }

  try {
    const doc = await Ticker.findOne({ ticker });
    const cached = doc?.history?.get(range);

    if (cached?.data?.length && isFresh(cached.updatedAt, RANGES[range].ttl)) {
      return res.json({ ticker, range, data: cached.data });
    }

    // Fetch from Yahoo Finance
    const { range: yahooRange, interval } = RANGES[range];
    const data = await getCandles(ticker, yahooRange, interval);

    if (!data) {
      return res.status(404).json({ error: 'No data available for this ticker/range' });
    }

    // Cache in MongoDB
    await Ticker.findOneAndUpdate(
      { ticker },
      { [`history.${range}`]: { data, updatedAt: new Date() } },
      { upsert: true }
    );

    res.json({ ticker, range, data });
  } catch (err) {
    console.error(`Error fetching history for ${ticker} (${range}):`, err.message);
    res.status(500).json({ error: 'Failed to fetch history' });
  }
});

app.get('/', (req, res) => {
  res.json({ status: 'ok', message: 'Pebble Stocks API' });
});

export default app;
