import { Agenda } from 'agenda';
import { MongoBackend } from '@agendajs/mongo-backend';
import Ticker from './models/Ticker.js';
import { getQuote } from './provider/yahoo.js';

export async function startWorker(mongoUri) {
  const agenda = new Agenda({
    backend: new MongoBackend({ address: mongoUri, collection: 'agendaJobs' }),
    processEvery: '1 minute',
  });

  agenda.define('refresh-quotes', async () => {
    const tickers = await Ticker.find({}).select('ticker');

    if (tickers.length === 0) {
      console.log('[worker] No tickers to refresh');
      return;
    }

    console.log(`[worker] Refreshing ${tickers.length} ticker(s)...`);

    for (const { ticker } of tickers) {
      try {
        const quote = await getQuote(ticker);
        if (quote) {
          await Ticker.findOneAndUpdate({ ticker }, { quote });
          console.log(`[worker] Refreshed ${ticker}: $${quote.price}`);
        }
      } catch (err) {
        console.error(`[worker] Failed to refresh ${ticker}:`, err.message);
      }
    }
  });

  await agenda.start();
  await agenda.every('15 minutes', 'refresh-quotes');

  console.log('[worker] Background refresh scheduled (every 15 min)');
  return agenda;
}
