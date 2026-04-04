import mongoose from 'mongoose';

const historyRangeSchema = new mongoose.Schema({
  data: { type: [[Number]], default: [] }, // [[timestamp, close], ...]
  updatedAt: { type: Date, default: null }
}, { _id: false });

const quoteSchema = new mongoose.Schema({
  price: Number,
  change: Number,
  changePercent: Number,
  high: Number,
  low: Number,
  open: Number,
  previousClose: Number,
  updatedAt: { type: Date, default: null }
}, { _id: false });

const tickerSchema = new mongoose.Schema({
  ticker: { type: String, required: true, unique: true, uppercase: true, index: true },
  quote: { type: quoteSchema, default: () => ({}) },
  history: {
    type: Map,
    of: historyRangeSchema,
    default: () => new Map()
  }
});

export default mongoose.model('Ticker', tickerSchema);
