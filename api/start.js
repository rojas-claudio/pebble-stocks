import mongoose from 'mongoose';
import app from './server.js';
import { startWorker } from './worker.js';

import dotenv from "dotenv";
dotenv.config({ path: ".env" });

const PORT = process.env.PORT || 3000;
const MONGO_URI = process.env.MONGO_URI;

async function start() {
  // Connect to MongoDB
  await mongoose.connect(MONGO_URI);
  console.log(`Connected to MongoDB at ${MONGO_URI}`);

  // Start background worker
  const agenda = await startWorker(MONGO_URI);

  // Start Express server
  app.listen(PORT, () => {
    console.log(`Pebble Stocks API running at http://localhost:${PORT}`);
  });

  // Graceful shutdown
  const shutdown = async () => {
    console.log('\nShutting down...');
    await agenda.stop();
    await mongoose.connection.close();
    process.exit(0);
  };

  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

start().catch((err) => {
  console.error('Startup failed:', err);
  process.exit(1);
});
