import dotenv from "dotenv";
import { fileURLToPath } from "url";
import { dirname, join } from "path";

const __dirname = dirname(fileURLToPath(import.meta.url));
dotenv.config({ path: join(__dirname, "../.env") });

const API_BASE = "https://finnhub.io/api/v1";
const API_KEY = process.env.FINNHUB_API_KEY;

async function finnhubFetch(path, params) {
    const url = new URL(`${API_BASE}${path}`);
    for (const [key, val] of Object.entries(params)) {
        url.searchParams.set(key, val);
    }

    const res = await fetch(url, {
        headers: { "X-Finnhub-Token": API_KEY },
    });

    if (!res.ok) {
        throw new Error(`Finnhub API error: ${res.status} ${res.statusText}`);
    }

    return res.json();
}

export async function getQuote(ticker) {
    const data = await finnhubFetch("/quote", { symbol: ticker });

    // Finnhub returns all zeros for invalid tickers
    if (data.c === 0 && data.h === 0 && data.l === 0) {
        return null;
    }

    return {
        price: data.c,
        change: data.d,
        changePercent: data.dp,
        high: data.h,
        low: data.l,
        open: data.o,
        previousClose: data.pc,
        updatedAt: new Date(),
    };
}

// Yahoo Finance for historical data (Finnhub candles require paid plan)
export async function getCandles(ticker, yahooRange, yahooInterval) {
    const url = `https://query1.finance.yahoo.com/v8/finance/chart/${encodeURIComponent(ticker)}?range=${yahooRange}&interval=${yahooInterval}`;
    const res = await fetch(url, {
        headers: { "User-Agent": "PebbleStocksAPI/1.0" },
    });

    if (!res.ok) {
        throw new Error(`Yahoo Finance error: ${res.status} ${res.statusText}`);
    }

    const data = await res.json();
    const result = data.chart?.result?.[0];

    if (!result?.timestamp) {
        return null;
    }

    const close = result.indicators.quote[0].close;
    // Return only [timestamp, close] pairs for minimal Pebble payload
    return result.timestamp
        .map((t, i) => [t, close[i]])
        .filter(([, c]) => c != null);
}
