#include <pebble.h>

#include "graph_layer.h"

GraphLayer *s_graph_layer;

GraphLayer *graph_layer_create(Window *window,
                              uint32_t data[],
                              char *timeframe) {
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);

    int y_offset = STATUS_BAR_LAYER_HEIGHT;
    int x_offset = STATUS_BAR_LAYER_HEIGHT / 2;
    int top_third = window_bounds.size.h / 3;

    GRect graph_bounds = GRect(window_bounds.origin.x + x_offset,
                               window_bounds.origin.y + top_third,
                               window_bounds.size.w,
                               window_bounds.size.h - top_third);

    s_graph_layer = layer_create(graph_bounds);

    return s_graph_layer;
}
