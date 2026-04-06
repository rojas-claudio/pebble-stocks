#pragma once

#include <pebble.h>
#include "../stocks.h"

typedef Layer GraphLayer;

GraphLayer *graph_layer_create(GRect bounds);
void        graph_layer_set_data(GraphLayer *layer, int32_t *closes,
                                 int count, const char *timeframe);
void        graph_layer_destroy(GraphLayer *layer);
