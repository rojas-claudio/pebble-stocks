#pragma once

#include <pebble.h>

typedef Layer InfoLayer;

InfoLayer *info_layer_create(GRect bounds);
void info_layer_set_data(InfoLayer *layer,
                         struct tm *time, int market_hours, int index, int total);
void info_layer_destroy(InfoLayer *layer);