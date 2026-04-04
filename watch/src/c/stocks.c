#include <pebble.h>

#include "stocks.h"

#include "windows/splash_window.h"
#include "windows/watchlist_window.h"
#include "windows/error_window.h"
#include "windows/detail_window.h"

#include "layers/graph_layer.h"

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

// static void inbox_dropped_callback(AppMessageResult reason, void *context) {
// }

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *type_tuple = dict_find(iterator, MESSAGE_KEY_Type);
    if (!type_tuple) {
        return;
    }

    MsgType msg_type = (MsgType)type_tuple->value->int32;

    if (!s_watchlist_initialized) {
        switch (msg_type) {
            case MSG_TYPE_READY: {
                // APP_LOG(APP_LOG_LEVEL_INFO, "READY received");
                splash_update_progress(25);
                break;
            }
            case MSG_TYPE_ERROR: {
                // TODO: Implement error logic
                break;
            }
            case MSG_TYPE_NOCONNECTION: {
                splash_deinit();
                error_init();
                break;
            }
            case MSG_TYPE_LOADED: {
                s_watchlist_initialized = true;
                splash_update_progress(100);
                splash_deinit();
                watchlist_window_init(s_watchlist, s_total_quotes);
                break;
            }
            case MSG_TYPE_SYMBOLDATA: {
                // APP_LOG(APP_LOG_LEVEL_INFO, "SYMBOLDATA received");
                Tuple *watchlist_position_tuple = dict_find(iterator, MESSAGE_KEY_WatchlistPosition);
                if (!watchlist_position_tuple) { return; }
                Tuple *watchlist_size_tuple = dict_find(iterator, MESSAGE_KEY_WatchlistSize);
                if (!watchlist_size_tuple) { return; }

                Tuple *symbol_tuple = dict_find(iterator, MESSAGE_KEY_Symbol);
                Tuple *price_tuple = dict_find(iterator, MESSAGE_KEY_Price);
                Tuple *change_tuple = dict_find(iterator, MESSAGE_KEY_Change);
                Tuple *change_percent_tuple = dict_find(iterator, MESSAGE_KEY_ChangePercent);
                Tuple *last_updated_tuple = dict_find(iterator, MESSAGE_KEY_LastUpdated);

                int change_val = (int)change_tuple->value->int32;
                int change_percent_val = (int)change_percent_tuple->value->int32;
                int last_updated_val = (int)last_updated_tuple->value->int32;

                if (!symbol_tuple || !price_tuple || !change_tuple || !change_percent_tuple || !last_updated_tuple) { return; }

                StockData_t quote;
                quote.position = watchlist_position_tuple->value->int32;
                strncpy(quote.symbol, symbol_tuple->value->cstring, sizeof(quote.symbol) - 1);
                snprintf(quote.price, sizeof(quote.price), "%d.%02d", (int)price_tuple->value->int32 / 100, (int)price_tuple->value->int32 % 100);
                snprintf(quote.change, sizeof(quote.change), "%s%d.%02d",
                        change_val > 0 ? "+" : (change_val < 0 ? "-" : ""),
                        abs(change_val) / 100, abs(change_val) % 100);
                snprintf(quote.changePercent, sizeof(quote.changePercent), "%s%d.%02d",
                        change_percent_val > 0 ? "+" : (change_percent_val < 0 ? "-" : ""),
                        abs(change_percent_val) / 100, abs(change_percent_val) % 100);

                time_t t = (time_t)last_updated_val;
                struct tm *tm_info = localtime(&t);
                strftime(quote.lastUpdated, sizeof(quote.lastUpdated), "%H:%M", tm_info);

                s_watchlist[quote.position] = quote;
                s_received_quotes++;
                s_total_quotes = watchlist_size_tuple->value->int32;
                if (s_received_quotes > 0) { // between 25 and 100% as quotes arrive
                    int progress = 25 + (75 * s_received_quotes / s_total_quotes);
                    splash_update_progress(progress);
                }
                break;
            }
            case MSG_TYPE_HISTORY_DATA: {
                Tuple *symbol_tuple = dict_find(iterator, MESSAGE_KEY_Symbol);
                Tuple *timeframe_tuple = dict_find(iterator, MESSAGE_KEY_Timeframe);
                Tuple *history_data_tuple = dict_find(iterator, MESSAGE_KEY_HistoryData);

                if (!symbol_tuple || !timeframe_tuple || !history_data_tuple) {
                    // TODO: error logic
                    return;
                }

                bool is_detail_active = window_stack_get_top_window() == detail_window_get_window();
                bool is_correct_symbol = symbol_tuple && strcmp(symbol_tuple->value->cstring, detail_window_get_symbol()) == 0;

                if (is_detail_active && is_correct_symbol) {

                }

                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        // Logic for handling updates to individual stocks
    }
}

static void stocks_deinit(void) {
    app_message_deregister_callbacks();
}

static void stocks_init(void) {
    // app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    // app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_inbox_received(inbox_received_callback);

    app_message_open(512, 64);

    splash_init();
}

int main(void) {
    stocks_init();
    app_event_loop();
    stocks_deinit();
}
