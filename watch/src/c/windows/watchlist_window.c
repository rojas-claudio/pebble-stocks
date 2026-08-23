/*
 * Pebble Stocks — watchlist menu of configured symbols.
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

#include "watchlist_window.h"
#include "detail_window.h"
#include "disclaimer_window.h"
#include "../stocks.h"

static Window       *s_watchlist_window;
static MenuLayer    *s_tickers_layer;
static int          s_watchlist_size = 0;

static uint16_t get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 2;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    if (section_index == 0) return s_watchlist_size;
    return 1; // section 1: Disclaimers
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    GRect bounds = layer_get_bounds(cell_layer);

    if (cell_index->section == 1) {
#ifdef PBL_COLOR
        graphics_context_set_text_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx,
            menu_cell_layer_is_highlighted(cell_layer) ? GColorLightGray : GColorBlack);
#endif
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        menu_cell_basic_draw(ctx, cell_layer, "Disclaimers", NULL, NULL);
        return;
    }

    StockData_t *quote = stocks_get_quote(cell_index->row);
    if (!quote) return;

    char title[18], subtitle[18];
    snprintf(title, sizeof(title), "%s", quote->symbol);
    snprintf(subtitle, sizeof(subtitle), "$%s", quote->price);

#ifdef PBL_COLOR
    GColor bg_color;
    if (quote->change[0] != '-' && quote->change[0] != '0') {
        bg_color = menu_cell_layer_is_highlighted(cell_layer) ? GColorIslamicGreen : GColorBlack;
    } else if (quote->change[0] == '-') {
        bg_color = menu_cell_layer_is_highlighted(cell_layer) ? GColorDarkCandyAppleRed : GColorBlack;
    } else {
        bg_color = menu_cell_layer_is_highlighted(cell_layer) ? GColorLightGray : GColorBlack;
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, bg_color);
#endif

    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
#if PBL_PLATFORM_EMERY || PBL_PLATFORM_GABBRO
    return 53;
#else
    return 42;
#endif
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (cell_index->section == 1) {
        disclaimer_window_init(NULL);
        return;
    }
    detail_window_init(cell_index->row);
}

static void on_quote_updated(int position) {
    if (s_tickers_layer) menu_layer_reload_data(s_tickers_layer);
}

static void watchlist_window_appear(Window *window) {
    stocks_on_quote_updated(on_quote_updated);
}

static void watchlist_window_disappear(Window *window) {
    stocks_on_quote_updated(NULL);
}

static void watchlist_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_tickers_layer = menu_layer_create(bounds);
    menu_layer_set_click_config_onto_window(s_tickers_layer, window);

#ifdef PBL_COLOR
    menu_layer_set_normal_colors(s_tickers_layer, GColorBlack, GColorWhite);
    menu_layer_set_highlight_colors(s_tickers_layer, GColorLightGray, GColorWhite);
#else
    menu_layer_set_normal_colors(s_tickers_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_tickers_layer, GColorBlack, GColorWhite);
#endif

    menu_layer_set_callbacks(s_tickers_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = get_num_sections_callback,
        .get_num_rows     = get_num_rows_callback,
        .draw_row         = draw_row_callback,
        .get_cell_height  = get_cell_height_callback,
        .select_click     = select_callback,
    });

    layer_add_child(window_layer, menu_layer_get_layer(s_tickers_layer));
}

static void watchlist_window_unload(Window *window) {
    menu_layer_destroy(s_tickers_layer);
}

void watchlist_window_deinit(void) {
    window_destroy(s_watchlist_window);
}

void watchlist_window_init(int quote_count) {
    s_watchlist_size = quote_count;

    s_watchlist_window = window_create();
    window_set_window_handlers(s_watchlist_window, (WindowHandlers) {
        .load      = watchlist_window_load,
        .unload    = watchlist_window_unload,
        .appear    = watchlist_window_appear,
        .disappear = watchlist_window_disappear,
    });
    window_set_background_color(s_watchlist_window, PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite));
    window_stack_push(s_watchlist_window, true);
}
