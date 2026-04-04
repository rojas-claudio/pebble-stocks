#include <pebble.h>

#include "error_window.h"

static Window *s_window;
static Layer *s_canvas_layer;

static GDrawCommandImage *s_error_icon_image;
static TextLayer *s_error_description_layer;

static void update_proc(Layer *layer, GContext *ctx) {
    GSize img_size = gdraw_command_image_get_bounds_size(s_error_icon_image);
    GRect bounds = layer_get_bounds(layer);

    const GEdgeInsets frame_insets = {
        .top = (bounds.size.h - img_size.h) / 2,
        .left = (bounds.size.w - img_size.w) / 2,
    };

    if (s_error_icon_image) {
        gdraw_command_image_draw(ctx, s_error_icon_image, grect_inset(bounds, frame_insets).origin);
    }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_error_icon_image = gdraw_command_image_create_with_resource(RESOURCE_ID_ERROR_SERVER_UNREACHABLE);
    if (!s_error_icon_image) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "[WATCH][ERROR] Failed to load error icon image");
    }

    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, update_proc);
    layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
    layer_destroy(s_canvas_layer);
    text_layer_destroy(s_error_description_layer);
    gdraw_command_image_destroy(s_error_icon_image);
    window_stack_remove(s_window, true);
}

void error_deinit(void) {
    window_destroy(s_window);
}

void error_init(void) {
    s_window = window_create();
    window_set_background_color(s_window, GColorBlack);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
}