/*
 * Pebble Stocks — scrolling disclaimer notice.
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

#include "disclaimer_window.h"

#define PERSIST_KEY_DISCLAIMER_ACCEPTED 1

#ifdef PBL_ROUND
#define PADDING_H  16
#define PADDING_V   8
#else
#define PADDING_H   4
#define PADDING_V   4
#endif

static const char *DISCLAIMER_TEXT =
    "FOR INFORMATIONAL USE ONLY\n\n"
    "Data is delayed a minimum of 2 minutes and may be inaccurate or incomplete. "
    "Prices are sourced from third-party providers and are not guaranteed.\n\n"
    "NOT FINANCIAL ADVICE\n\n"
    "This app is for informational purposes only. Do not make investment or trading "
    "decisions based on data shown here. The developer assumes no liability for "
    "losses incurred from use of this app.";

static Window      *s_disclaimer_window;
static ScrollLayer *s_scroll_layer;
static TextLayer   *s_text_layer;

static void (*s_on_accepted)(void) = NULL;

// -------------------------------------------------------------------------
// Button handlers
// -------------------------------------------------------------------------

#define SCROLL_STEP 30

static int s_scroll_offset = 0;
static int s_content_h     = 0;

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    s_scroll_offset -= SCROLL_STEP;
    if (s_scroll_offset < 0) s_scroll_offset = 0;
    scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, -s_scroll_offset), true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    GRect frame      = layer_get_frame(scroll_layer_get_layer(s_scroll_layer));
    int   max_offset = s_content_h - frame.size.h;
    s_scroll_offset += SCROLL_STEP;
    if (s_scroll_offset > max_offset) s_scroll_offset = max_offset;
    scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, -s_scroll_offset), true);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    window_stack_remove(s_disclaimer_window, true);
    if (s_on_accepted) s_on_accepted();
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP,     up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN,   down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// -------------------------------------------------------------------------
// Window lifecycle
// -------------------------------------------------------------------------

static void disclaimer_window_load(Window *window) {
    s_scroll_offset = 0;
    persist_write_bool(PERSIST_KEY_DISCLAIMER_ACCEPTED, true);

    Layer *window_layer = window_get_root_layer(window);
    GRect  bounds       = layer_get_bounds(window_layer);

    int text_w = bounds.size.w - 2 * PADDING_H;

    // Measure the actual rendered text height to avoid over-scrolling.
#ifdef PBL_ROUND
    GTextAlignment text_align = GTextAlignmentCenter;
    int text_y = bounds.size.h / 2;  // widest point of the circle
#else
    GTextAlignment text_align = GTextAlignmentLeft;
    int text_y = PADDING_V;
#endif
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    GSize text_size = graphics_text_layout_get_content_size(
        DISCLAIMER_TEXT, font,
        GRect(0, 0, text_w, 2000),
        GTextOverflowModeWordWrap, text_align);
    int text_h  = text_size.h + PADDING_V;
#ifdef PBL_ROUND
    s_content_h = text_y + text_h + text_y;  // trailing space mirrors leading offset so last line reaches centre
#else
    s_content_h = text_y + text_h;
#endif

    s_scroll_layer = scroll_layer_create(bounds);
    scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, s_content_h));
    scroll_layer_set_shadow_hidden(s_scroll_layer, true);

    s_text_layer = text_layer_create(GRect(PADDING_H, text_y, text_w, text_h));
    text_layer_set_text(s_text_layer, DISCLAIMER_TEXT);
    text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text_alignment(s_text_layer, text_align);
    text_layer_set_background_color(s_text_layer, GColorClear);
    text_layer_set_font(s_text_layer, font);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_text_layer, GColorWhite);
#else
    text_layer_set_text_color(s_text_layer, GColorBlack);
#endif

    scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
    layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void disclaimer_window_unload(Window *window) {
    text_layer_destroy(s_text_layer);
    scroll_layer_destroy(s_scroll_layer);
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

bool disclaimer_window_needs_display(void) {
    return !persist_read_bool(PERSIST_KEY_DISCLAIMER_ACCEPTED);
}

void disclaimer_window_init(void (*on_accepted)(void)) {
    s_on_accepted = on_accepted;

    s_disclaimer_window = window_create();
    window_set_click_config_provider(s_disclaimer_window, click_config_provider);
    window_set_window_handlers(s_disclaimer_window, (WindowHandlers) {
        .load   = disclaimer_window_load,
        .unload = disclaimer_window_unload,
    });
#ifdef PBL_COLOR
    window_set_background_color(s_disclaimer_window, GColorBlack);
#else
    window_set_background_color(s_disclaimer_window, GColorWhite);
#endif
    window_stack_push(s_disclaimer_window, true);
}

void disclaimer_window_deinit(void) {
    window_destroy(s_disclaimer_window);
}
