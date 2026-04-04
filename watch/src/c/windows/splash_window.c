#include <pebble.h>

#include "splash_window.h"

static Window *s_window;
static ProgressLayer *s_progress_layer;

static int s_progress;

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_progress_layer = progress_layer_create(GRect((bounds.size.w - PROGRESS_LAYER_WINDOW_WIDTH) / 2, bounds.size.h / 2 - 3, PROGRESS_LAYER_WINDOW_WIDTH, 6));
    progress_layer_set_progress(s_progress_layer, 0);
    progress_layer_set_corner_radius(s_progress_layer, 2);
    progress_layer_set_foreground_color(s_progress_layer, PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorWhite));
    progress_layer_set_background_color(s_progress_layer, GColorLightGray);

    layer_add_child(window_layer, s_progress_layer);
}

static void window_unload(Window *window) {
    progress_layer_destroy(s_progress_layer);
    window_destroy(s_window);
}

void splash_update_progress(int progress_percent) {
    if (!s_progress_layer) {
        return;
    }
    s_progress = progress_percent;
    progress_layer_set_progress(s_progress_layer, s_progress);
}

void splash_deinit(void) {
    window_stack_remove(s_window, true);
}

void splash_init(void) {
    s_window = window_create();
    window_set_background_color(s_window, GColorBlack);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
}