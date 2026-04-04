#pragma once

#include <pebble.h>

#include "../stocks.h"

#define GRAPH_MAX_POINTS 255

typedef Layer GraphLayer;

GraphLayer *graph_layer_create(Window *window, uint32_t data[], char *timeframe);
