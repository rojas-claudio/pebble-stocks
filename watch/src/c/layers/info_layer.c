#include <pebble.h>

#include "info_layer.h"

typedef struct {
    bool has_time;
    struct tm time;
    int market_hours;
    int index;
    int total;
} InfoLayerData;

static void info_layer_update_proc(Layer *layer, GContext *ctx) {
    InfoLayerData *d = layer_get_data(layer);
    GRect bounds = layer_get_bounds(layer);
#if PBL_PLATFORM_EMERY || PBL_PLATFORM_GABBRO
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    int y = bounds.size.h - 18 - 4;
#else
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    int y = bounds.size.h - 14 - 4;
#endif

    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
    graphics_context_set_stroke_width(ctx, 1);
    for (int x = 0; x < bounds.size.w; x++) {
        if (x % 3 == 0) graphics_draw_pixel(ctx, GPoint(x, bounds.size.h - 1));
    }

    graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
    if (d->has_time) {
        static char time_buffer[16];
        strftime(time_buffer, sizeof(time_buffer), "%l:%M %p", &d->time);
        graphics_draw_text(ctx, time_buffer, font, GRect(0, y, bounds.size.w, bounds.size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }

#ifndef PBL_ROUND
    if (d->index >= 0 && d->total > 0) {
        static char status_buffer[16];
        snprintf(status_buffer, sizeof(status_buffer), "%d/%d", d->index, d->total);
        graphics_draw_text(ctx, status_buffer, font, GRect(0 - 4, y, bounds.size.w, bounds.size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    }

    static char open_buffer[8];
    if (d->market_hours == 0) snprintf(open_buffer, sizeof(open_buffer), "PRE");
    if (d->market_hours == 1) snprintf(open_buffer, sizeof(open_buffer), "OPEN");
    if (d->market_hours == 2) snprintf(open_buffer, sizeof(open_buffer), "POST");
    if (d->market_hours == 3) snprintf(open_buffer, sizeof(open_buffer), "CLOSED");

    graphics_draw_text(ctx, open_buffer, font, GRect(4, y, bounds.size.w, bounds.size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
#endif
}

InfoLayer *info_layer_create(GRect bounds) {
    InfoLayer      *layer  = layer_create_with_data(bounds, sizeof(InfoLayerData));
    InfoLayerData  *d      = layer_get_data(layer);
    layer_set_update_proc(layer, info_layer_update_proc);
    return layer;
}

void info_layer_set_data(InfoLayer *layer,
                         struct tm *time, int market_hours, int index, int total) {
    InfoLayerData *d = layer_get_data(layer);

    d->has_time    = (time != NULL);
    if (time) d->time = *time;
    d->market_hours = market_hours;
    d->index       = index;
    d->total       = total;
}

void info_layer_destroy(InfoLayer *layer) {
    layer_destroy(layer);
}