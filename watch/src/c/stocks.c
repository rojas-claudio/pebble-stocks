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

#include <pebble.h>
#include <time.h>

#include "stocks.h"

#include "windows/splash_window.h"
#include "windows/watchlist_window.h"
#include "windows/error_window.h"

typedef enum {
    MSG_TYPE_ERROR = 0,
    MSG_TYPE_NOCONNECTION,
    MSG_TYPE_READY,
    MSG_TYPE_LOADED,
    MSG_TYPE_SYMBOLDATA,
    MSG_TYPE_HISTORY_REQUEST,
    MSG_TYPE_HISTORY_DATA,
    MSG_TYPE_REFRESH
} MsgType;

static StockData_t s_watchlist[10];
static int s_received_quotes = 0;
static int s_total_quotes = 0;
static bool s_watchlist_initialized = false;

static StockQuoteUpdatedCallback s_quote_cb = NULL;
static StockHistoryUpdatedCallback s_history_cb = NULL;

// -------------------------------------------------------------------------
// Public
// -------------------------------------------------------------------------

StockData_t *stocks_get_quote(int position) {
    if (position < 0 || position >= s_total_quotes) return NULL;
    return &s_watchlist[position];
}

void stocks_request_refresh(void) {
    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result != APP_MSG_OK) return;

    dict_write_int8(iter, MESSAGE_KEY_Type, MSG_TYPE_REFRESH);

    app_message_outbox_send();
}

void stocks_request_history(const char *symbol, const char *timeframe) {
    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result != APP_MSG_OK) return;

    dict_write_int8(iter, MESSAGE_KEY_Type, MSG_TYPE_HISTORY_REQUEST);
    dict_write_cstring(iter, MESSAGE_KEY_Symbol, symbol);
    dict_write_cstring(iter, MESSAGE_KEY_Timeframe, timeframe);

    APP_LOG(APP_LOG_LEVEL_INFO, "[WATCH][HISTORY] Requesting Symbol: %s, Timeframe: %s",
            symbol, timeframe);

    app_message_outbox_send();
}

void stocks_on_quote_updated(StockQuoteUpdatedCallback cb) {
    s_quote_cb = cb;
}

void stocks_on_history_updated(StockHistoryUpdatedCallback cb) {
    s_history_cb = cb;
}

// -------------------------------------------------------------------------
// Handlers for incoming data
// -------------------------------------------------------------------------

static void handle_symbol_data(DictionaryIterator *iterator) {
    Tuple *position_tuple       = dict_find(iterator, MESSAGE_KEY_WatchlistPosition);
    Tuple *size_tuple           = dict_find(iterator, MESSAGE_KEY_WatchlistSize);
    Tuple *symbol_tuple         = dict_find(iterator, MESSAGE_KEY_Symbol);
    Tuple *price_tuple          = dict_find(iterator, MESSAGE_KEY_Price);
    Tuple *change_tuple         = dict_find(iterator, MESSAGE_KEY_Change);
    Tuple *change_percent_tuple = dict_find(iterator, MESSAGE_KEY_ChangePercent);
    Tuple *hours_tuple          = dict_find(iterator, MESSAGE_KEY_Hours);
    Tuple *last_updated_tuple   = dict_find(iterator, MESSAGE_KEY_LastUpdated);

    if (!position_tuple         || 
        !size_tuple             || 
        !symbol_tuple           || 
        !price_tuple            || 
        !change_tuple           || 
        !change_percent_tuple   || 
        !hours_tuple            ||
        !last_updated_tuple) {
        return;
    }

    int position            = (int)position_tuple->value->int8;
    int size                = (int)size_tuple->value->int8;
    int hours               = (int)hours_tuple->value->int8;
    int change_val          = (int)change_tuple->value->int32;
    int change_percent_val  = (int)change_percent_tuple->value->int32;
    int last_updated_val    = (int)last_updated_tuple->value->int32;

    StockData_t *quote = &s_watchlist[position];
    quote->position = position;
    quote->size = size;
    quote->marketHours = hours;

    strncpy(quote->symbol, symbol_tuple->value->cstring, sizeof(quote->symbol) - 1);
    
    snprintf(quote->price, sizeof(quote->price), "%d.%02d", (int)price_tuple->value->int32 / 100, (int)price_tuple->value->int32 % 100);
    snprintf(quote->change, sizeof(quote->change), "%s%d.%02d",
             change_val > 0 ? "+" : (change_val < 0 ? "-" : ""),
             abs(change_val) / 100, abs(change_val) % 100);
    snprintf(quote->changePercent, sizeof(quote->changePercent), "%s%d.%02d",
             change_percent_val > 0 ? "+" : (change_percent_val < 0 ? "-" : ""),
             abs(change_percent_val) / 100, abs(change_percent_val) % 100);

    time_t t = (time_t)last_updated_val;
    struct tm *tm_info = localtime(&t);
    strftime(quote->lastUpdated, sizeof(quote->lastUpdated), "%H:%M", tm_info);

    s_total_quotes = size;
    s_received_quotes++;
}

static void handle_history_data(DictionaryIterator *iterator) {
    Tuple *position_tuple       = dict_find(iterator, MESSAGE_KEY_WatchlistPosition);
    Tuple *symbol_tuple         = dict_find(iterator, MESSAGE_KEY_Symbol);
    Tuple *timeframe_tuple      = dict_find(iterator, MESSAGE_KEY_Timeframe);
    Tuple *history_data_tuple   = dict_find(iterator, MESSAGE_KEY_HistoryData);
    Tuple *history_size_tuple   = dict_find(iterator, MESSAGE_KEY_HistoryDataSize);


    if ( !position_tuple || !symbol_tuple || !timeframe_tuple || !history_data_tuple || !history_size_tuple) {
        return;
    }

    int position            = (int)position_tuple->value->int32;
    const char *symbol            = symbol_tuple->value->cstring;
    const char *timeframe         = timeframe_tuple->value->cstring;
    int history_size        = (int)history_size_tuple->value->int32;

    StockData_t *quote = &s_watchlist[position];
    if (quote->history) {
        free(quote->history);
    }
    quote->history = malloc(sizeof(StockHistory_t));
    int point_count = history_size;
    if (point_count > MAX_HISTORY_POINTS) point_count = MAX_HISTORY_POINTS;
    int max_from_bytes = (int)history_data_tuple->length / (int)sizeof(int32_t);
    if (point_count > max_from_bytes) point_count = max_from_bytes;
    memcpy(quote->history->closes, history_data_tuple->value->data, point_count * sizeof(int32_t));
    quote->history->count = point_count;
    strncpy(quote->history->symbol, symbol, sizeof(quote->history->symbol) - 1);
    strncpy(quote->history->timeframe, timeframe, sizeof(quote->history->timeframe) - 1);
}

// -------------------------------------------------------------------------
// AppMessage
// -------------------------------------------------------------------------

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    Tuple *type_tuple = dict_find(iterator, MESSAGE_KEY_Type);
    if (!type_tuple) {
        return;
    }

    MsgType msg_type = (MsgType)type_tuple->value->int32;

    switch (msg_type) {
        default: {
            return;
        }
    }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *type_tuple = dict_find(iterator, MESSAGE_KEY_Type);
    if (!type_tuple) {
        return;
    }

    MsgType msg_type = (MsgType)type_tuple->value->int32;

    switch (msg_type) {
        case MSG_TYPE_READY: {
            splash_update_progress(25);
            break;
        }
        case MSG_TYPE_ERROR: {
            break;
        }
        case MSG_TYPE_NOCONNECTION: {
            if (!s_watchlist_initialized) {
                splash_deinit();
                error_init();
            }
            break;
        }
        case MSG_TYPE_LOADED: {
            if (!s_watchlist_initialized) {
                s_received_quotes = 0;
                s_watchlist_initialized = true;
                splash_update_progress(100);
                splash_deinit();
                watchlist_window_init(s_total_quotes);
            }
            break;
        }
        case MSG_TYPE_SYMBOLDATA: {
            handle_symbol_data(iterator);

            Tuple *pos_tuple = dict_find(iterator, MESSAGE_KEY_WatchlistPosition);
            int position = pos_tuple ? (int)pos_tuple->value->int32 : -1;

            if (!s_watchlist_initialized) {
                if (s_received_quotes > 0) {
                    int progress = 25 + (75 * s_received_quotes / s_total_quotes);
                    splash_update_progress(progress);
                }
            } else {
                if (s_quote_cb) s_quote_cb(position);
            }
            break;
        }
        case MSG_TYPE_HISTORY_DATA: {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Received history data");
            handle_history_data(iterator);

            Tuple *pos_tuple = dict_find(iterator, MESSAGE_KEY_WatchlistPosition);
            int position = pos_tuple ? (int)pos_tuple->value->int32 : -1;
            if (s_history_cb) s_history_cb(position);

            break;
        }
        default: {
            return;
        }
    }
}

// -------------------------------------------------------------------------
// Stocks
// -------------------------------------------------------------------------

static void stocks_deinit(void) {
    app_message_deregister_callbacks();
}

static void stocks_init(void) {
    app_message_register_outbox_sent(outbox_sent_callback);
    app_message_register_inbox_received(inbox_received_callback);

    app_message_open(512, 64);

    splash_init();
}

int main(void) {
    stocks_init();
    app_event_loop();
    stocks_deinit();
}
