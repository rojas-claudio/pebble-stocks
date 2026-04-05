#include <pebble.h>

#include "detail_window.h"
#include "../stocks.h"

static Window *s_detail_window;
static StatusBarLayer *s_status_bar_layer;

// DEBUG
static Layer *s_debug_divider_layer;

/*
    -----------------------------
    |            |              |
    |      1     |       2      |
    |            |              |
    |            |              |
    =============================
    |                           |
    |                           |
    |             3             |
    |                           |
    |                           |
    |                           |
    =============================
*/

// 1
static TextLayer *s_symbol_layer;
static TextLayer *s_price_layer;
static char s_symbol_buffer[18];
static char s_price_buffer[18]; // "$" + up to 16-char price

// 2
static TextLayer *s_change_layer;
static TextLayer *s_change_percent_layer;
static char s_change_buffer[18];
static char s_change_percent_buffer[18];

// 3
// static TextLayer *s_high_layer;
// static TextLayer *s_low_layer;

static StockData_t s_current_quote;

Window *detail_window_get_window(void) {
    return s_detail_window;
}

const char *detail_window_get_symbol(void) {
    return s_current_quote.symbol;
}

// DEBUG: draws segment dividers matching the layout comment above
static void debug_divider_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int top_third = bounds.size.h / 3;

    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 1);

    // Horizontal line at the bottom of the top third
    graphics_draw_line(ctx,
        GPoint(0, top_third),
        GPoint(bounds.size.w, top_third));

    // Vertical line splitting the top third in half
    graphics_draw_line(ctx,
        GPoint(bounds.size.w / 2, 0),
        GPoint(bounds.size.w / 2, top_third));
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Handle up button click
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Handle down button click
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void detail_window_load(Window *window) {
    int y_offset = STATUS_BAR_LAYER_HEIGHT;
    int x_offset = STATUS_BAR_LAYER_HEIGHT / 2;

    // Window Layer
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);

    // Status Bar
    s_status_bar_layer = status_bar_layer_create();
    status_bar_layer_set_colors(s_status_bar_layer, GColorBlack, GColorWhite);
    status_bar_layer_set_separator_mode(s_status_bar_layer, StatusBarLayerSeparatorModeDotted);

    // Segment 1
    GRect symbol_bounds = GRect(window_bounds.origin.x + x_offset,
                                window_bounds.origin.y + y_offset,
                                window_bounds.size.w / 2,
                                (window_bounds.size.h - y_offset) / 6);

    s_symbol_layer = text_layer_create(symbol_bounds);
    snprintf(s_symbol_buffer, sizeof(s_symbol_buffer), "$%s", s_current_quote.symbol);
    text_layer_set_text(s_symbol_layer, s_symbol_buffer);
    text_layer_set_background_color(s_symbol_layer, GColorClear);
    text_layer_set_text_color(s_symbol_layer, GColorWhite);
    text_layer_set_text_alignment(s_symbol_layer, GTextAlignmentLeft);

    GRect price_bounds = GRect(window_bounds.origin.x + x_offset,
                               symbol_bounds.origin.y + symbol_bounds.size.h,
                               window_bounds.size.w / 2,
                               (window_bounds.size.h - y_offset) / 3 - symbol_bounds.size.h);

    s_price_layer = text_layer_create(price_bounds);
    snprintf(s_price_buffer, sizeof(s_price_buffer), "$%s", s_current_quote.price);
    text_layer_set_text(s_price_layer, s_price_buffer);
    text_layer_set_background_color(s_price_layer, GColorClear);
    text_layer_set_text_color(s_price_layer, GColorWhite);
    text_layer_set_text_alignment(s_price_layer, GTextAlignmentLeft);

    // Segment 2
    GRect change_percent_bounds = GRect((window_bounds.size.w / 2) - x_offset,
                                window_bounds.origin.y + y_offset,
                                window_bounds.size.w / 2,
                                (window_bounds.size.h - y_offset) / 6);

    bool is_negative = s_current_quote.change[0] == '-';
    bool is_zero = s_current_quote.change[0] == '0';
    GColor change_color = is_negative ? GColorRed : (is_zero ? GColorLightGray : GColorGreen);

    s_change_percent_layer = text_layer_create(change_percent_bounds);
    snprintf(s_change_percent_buffer, sizeof(s_change_percent_buffer), "%s%%", s_current_quote.changePercent);
    text_layer_set_text(s_change_percent_layer, s_change_percent_buffer);
    text_layer_set_background_color(s_change_percent_layer, GColorClear);
    text_layer_set_text_color(s_change_percent_layer, change_color);
    text_layer_set_text_alignment(s_change_percent_layer, GTextAlignmentRight);

    GRect change_bounds = GRect((window_bounds.size.w / 2) - x_offset,
                                change_percent_bounds.origin.y + change_percent_bounds.size.h,
                                window_bounds.size.w / 2,
                                (window_bounds.size.h - y_offset) / 3 - change_percent_bounds.size.h);

    s_change_layer = text_layer_create(change_bounds);
    text_layer_set_text(s_change_layer, s_current_quote.change);
    text_layer_set_background_color(s_change_layer, GColorClear);
    text_layer_set_text_color(s_change_layer, change_color);
    text_layer_set_text_alignment(s_change_layer, GTextAlignmentRight);


    // Segment 3

    // DEBUG: full-window transparent layer drawn on top to render segment dividers
    GRect usable_bounds = GRect(window_bounds.origin.x,
                                window_bounds.origin.y + y_offset,
                                window_bounds.size.w,
                                window_bounds.size.h - y_offset);
    s_debug_divider_layer = layer_create(usable_bounds);
    layer_set_update_proc(s_debug_divider_layer, debug_divider_update_proc);

#if PBL_PLATFORM_EMERY || PBL_PLATFORM_GABBRO
    text_layer_set_font(s_symbol_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_font(s_price_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    text_layer_set_font(s_change_percent_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_font(s_change_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
#else
    text_layer_set_font(s_symbol_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_font(s_price_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_font(s_change_percent_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_font(s_change_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
#endif

    layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_symbol_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_price_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_change_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_change_percent_layer));
    // layer_add_child(window_layer, s_debug_divider_layer);
}

static void detail_window_unload(Window *window) {
    layer_destroy(s_debug_divider_layer);
    status_bar_layer_destroy(s_status_bar_layer);
    text_layer_destroy(s_symbol_layer);
    text_layer_destroy(s_price_layer);
    text_layer_destroy(s_change_layer);
    text_layer_destroy(s_change_percent_layer);
}

void detail_window_deinit(void) {
    window_destroy(s_detail_window);
}

void detail_window_init(int position) {
    char *default_timeframe = "1W"; // will be configurable later

    StockData_t *quote = stocks_get_quote(position);
    if (!quote) {
        return;
    }
    s_current_quote = *quote;

    s_detail_window = window_create();
    window_set_click_config_provider(s_detail_window, click_config_provider);
    window_set_window_handlers(s_detail_window, (WindowHandlers) {
        .load = detail_window_load,
        .unload = detail_window_unload,
    });
    
#ifdef PBL_COLOR
    window_set_background_color(s_detail_window, GColorBlack);
#else
    window_set_background_color(s_detail_window, GColorWhite);
#endif
    stocks_request_history(quote->symbol, default_timeframe);

    window_stack_push(s_detail_window, true);
}
