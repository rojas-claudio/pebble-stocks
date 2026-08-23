/*
 * Pebble Stocks — Mongoose schema for cached quotes and history.
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
  marketHours: Number, // 0 = pre-market, 1 = regular, 2 = post-market, 3 = closed
  previousClose: Number,
  updatedAt: { type: Date, default: null }
}, { _id: false });

const tickerSchema = new mongoose.Schema({
  ticker: { type: String, required: true, unique: true, uppercase: true, index: true },
  // Last time a client asked for this ticker. The cron refresh only re-fetches
  // recently-requested symbols, so entries nobody looks at fall out of rotation
  // instead of being refreshed forever. Indexed: refresh filters and sorts on it.
  lastAccessedAt: { type: Date, default: Date.now, index: true },
  quote: { type: quoteSchema, default: () => ({}) },
  history: {
    type: Map,
    of: historyRangeSchema,
    default: () => new Map()
  }
});

export default mongoose.model('Ticker', tickerSchema);
