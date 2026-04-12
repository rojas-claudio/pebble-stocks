import dotenv from "dotenv";
import { fileURLToPath } from "url";
import { dirname, join } from "path";

const __dirname = dirname(fileURLToPath(import.meta.url));
dotenv.config({ path: join(__dirname, "../.env") });

const API_BASE = "https://query1.finance.yahoo.com/v8/finance/chart";

const HISTORY_CONFIG = {
  '1D':  { range: '1d',  interval: '5m',  ttl: 5 },
  '1W':  { range: '5d',  interval: '1d', ttl: 30 },
  '1M':  { range: '1mo', interval: '1d',  ttl: 1440 },
  '3M':  { range: '3mo', interval: '1d',  ttl: 1440 },
  'YTD': { range: 'ytd', interval: '1d',  ttl: 1440 },
  '1Y':  { range: '1y',  interval: '1d',  ttl: 1440 },
};

const MARKET_HOURS = {
    PRE: 0,
    OPEN: 1,
    POST: 2,
    CLOSED: 3
};

async function yahooFetch(symbol, params) {
    const url = new URL(`${API_BASE}/${encodeURIComponent(symbol)}`);
    for (const [key, val] of Object.entries(params)) {
        url.searchParams.set(key, val);
    }

    const res = await fetch(url);

    if (!res.ok) {
        throw new Error(`Yahoo Finance error: ${res.status} ${res.statusText}`);
    }

    const data = await res.json();
    const result = data.chart?.result?.[0];

    if (!result) {
        throw new Error(`No data returned for ${symbol}`);
    }

    return result;
}

export async function getQuote(ticker) {
    const result = await yahooFetch(ticker, { range: '1d', interval: '1d' });

    const meta = result.meta;
    if (!meta?.regularMarketPrice) return null;

    const price = meta.regularMarketPrice;
    const previousClose = meta.previousClose ?? meta.chartPreviousClose;
    const change = meta.regularMarketChange ?? (price - previousClose);
    const changePercent = meta.regularMarketChangePercent ?? (change / previousClose * 100);
    var marketHours = MARKET_HOURS.CLOSED;
    
    for (var i = 0; i < meta.currentTradingPeriod.length; i++) {
        var period = meta.currentTradingPeriod[i];
        if (Date.now() >= period.start * 1000 && Date.now() < period.end * 1000) {
            marketHours = i; // 0=pre, 1=open, 2=post
            break;
        }
    }

    return {
        price,
        change,
        changePercent,
        marketHours,
        high: meta.regularMarketDayHigh ?? null,
        low: meta.regularMarketDayLow ?? null,
        open: meta.regularMarketOpen ?? null,
        previousClose,
        updatedAt: new Date(),
    };
}

export async function getHistory(ticker, range) {
    const config = HISTORY_CONFIG[range];
    if (!config) throw new Error(`Unknown range: ${range}`);

    const result = await yahooFetch(ticker, {
        range: config.range,
        interval: config.interval,
    });

    if (!result.timestamp) return null;

    const closes = result.indicators.quote[0].close;

    // Get [timestamp, close] pairs, dropping nulls (market closed periods)
    let data = result.timestamp
        .map((t, i) => [t, closes[i]])
        .filter(([, c]) => c != null);

    // Downsample to maximum 120 points
    const maxPoints = 100;
    if (data.length > maxPoints) {
        const step = Math.ceil(data.length / maxPoints);
        data = data.filter((_, i) => i % step === 0);
    }

    return data;
}
