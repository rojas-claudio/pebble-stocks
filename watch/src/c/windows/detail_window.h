#pragma once

#include <pebble.h>

#include "../stocks.h"

Window *detail_window_get_window(void);
const char *detail_window_get_symbol(void);

void detail_window_deinit(void);
void detail_window_init(StockData_t *quote);
