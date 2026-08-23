/*
 * Pebble Stocks — price history graph layer.
 * Copyright (C) 2026 Claudio Rojas
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License, version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <pebble.h>

#include "graph_layer.h"

typedef struct {
    int32_t closes[MAX_HISTORY_POINTS];
    int     count;
    char    timeframe[4];
} GraphLayerData;

static void graph_layer_update_proc(Layer *layer, GContext *ctx) {
    GraphLayerData *d      = layer_get_data(layer);
    GRect           bounds = layer_get_bounds(layer);

    if (!d || d->count < 2) {
        GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
        int y = ((bounds.size.h) / 2) - 12;
        GRect text_rect = GRect(0, y, bounds.size.w, y);
        graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
        graphics_draw_text(ctx, "Loading...",
            font,
            text_rect,
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentCenter,
            NULL);
        return;
    }

#if defined(PBL_PLATFORM_GABBRO)
    int padding = 4;
#else
    int padding = STATUS_BAR_LAYER_HEIGHT / 2;
#endif

    // Dotted x and y axes
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
    graphics_context_set_stroke_width(ctx, 1);
    for (int y = padding; y <= bounds.size.h - padding; y++) {
        if (y % 3 == 0) graphics_draw_pixel(ctx, GPoint(padding, y));
    }
    for (int x = padding; x <= bounds.size.w - padding; x++) {
        if (x % 3 == 0) graphics_draw_pixel(ctx, GPoint(x, bounds.size.h - padding));
    }

    // Find the price range
    int32_t min_val = d->closes[0];
    int32_t max_val = d->closes[0];
    for (int i = 1; i < d->count; i++) {
        if (d->closes[i] < min_val) min_val = d->closes[i];
        if (d->closes[i] > max_val) max_val = d->closes[i];
    }

    int32_t range = max_val - min_val;
    if (range == 0) range = 1;  // Flat line — avoid division by zero

    int plot_w  = bounds.size.w - 2 * padding;
    int plot_h  = bounds.size.h - 2 * padding;

    // Green if current >= open, red otherwise; black on B&W platforms
    bool is_up = d->closes[d->count - 1] >= d->closes[0];

    GColor line_color = PBL_IF_COLOR_ELSE(is_up ? GColorGreen : GColorRed, GColorBlack);

    graphics_context_set_stroke_color(ctx, line_color);
    graphics_context_set_stroke_width(ctx, 2);

    for (int i = 1; i < d->count; i++) {
        int x0 = padding + (i - 1) * plot_w / (d->count - 1);
        int x1 = padding + i       * plot_w / (d->count - 1);
        int y0 = padding + plot_h - (int)((d->closes[i - 1] - min_val) * plot_h / range);
        int y1 = padding + plot_h - (int)((d->closes[i]     - min_val) * plot_h / range);
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
    }

    // Price + timeframe labels
    graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
    GFont small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    graphics_draw_text(ctx, d->timeframe, small_font,
        GRect(bounds.size.w - padding - 26,
              bounds.size.h - padding - 18,
              26,
              18),
        GTextOverflowModeTrailingEllipsis,
        PBL_IF_RECT_ELSE(GTextAlignmentRight, GTextAlignmentCenter), NULL);
}

GraphLayer *graph_layer_create(GRect bounds) {
    GraphLayer     *layer = layer_create_with_data(bounds, sizeof(GraphLayerData));
    GraphLayerData *d     = layer_get_data(layer);
    memset(d, 0, sizeof(GraphLayerData));
    layer_set_update_proc(layer, graph_layer_update_proc);
    return layer;
}

void graph_layer_set_data(GraphLayer *layer, int32_t *closes, int count, const char *timeframe) {
    GraphLayerData *d = layer_get_data(layer);
    int n = count < MAX_HISTORY_POINTS ? count : MAX_HISTORY_POINTS;
    memcpy(d->closes, closes, n * sizeof(int32_t));
    d->count = n;
    strncpy(d->timeframe, timeframe, sizeof(d->timeframe) - 1);
    d->timeframe[sizeof(d->timeframe) - 1] = '\0';
}

void graph_layer_destroy(GraphLayer *layer) {
    layer_destroy(layer);
}
