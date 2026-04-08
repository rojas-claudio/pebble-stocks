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
    int32_t position;
    int32_t size;
    char symbol[16];
    char price[16];
    char change[16];
    char changePercent[16];
    char lastUpdated[16];
    StockHistory_t *history;
} StockData_t;

// callbacks
// typedef void (*StockQuoteUpdatedCallback)(int position);
// typedef void (*StockHistoryUpdatedCallback)(const char *symbol);

// void stocks_on_quote_updated(StockQuoteUpdatedCallback cb);
// void stocks_on_history_updated(StockHistoryUpdatedCallback cb);

// accessors
StockData_t *stocks_get_quote(int position);
StockHistory_t *stocks_get_history(const char *symbol);

// actions
void stocks_request_refresh(void);
void stocks_request_history(const char *symbol, const char *timeframe);
