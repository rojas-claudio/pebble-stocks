/*
 * Pebble Stocks — app entry, AppMessage handling and watchlist state.
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

#pragma once

#include <pebble.h>

#define MAX_HISTORY_POINTS 100

typedef struct StockHistory {
    char symbol[16];
    char timeframe[4];
    int32_t closes[MAX_HISTORY_POINTS];
    int count;
} StockHistory_t;

typedef struct StockData {
    int8_t position;
    int8_t size;
    int8_t marketHours;
    char symbol[16];
    char price[16];
    char change[16];
    char changePercent[16];
    char lastUpdated[16];
    StockHistory_t *history;
} StockData_t;

// callbacks
typedef void (*StockQuoteUpdatedCallback)(int position);
typedef void (*StockHistoryUpdatedCallback)(int position);

void stocks_on_quote_updated(StockQuoteUpdatedCallback cb);
void stocks_on_history_updated(StockHistoryUpdatedCallback cb);

// accessors
StockData_t *stocks_get_quote(int position);
StockHistory_t *stocks_get_history(const char *symbol);

// actions
void stocks_request_refresh(void);
void stocks_request_history(const char *symbol, const char *timeframe);
