#include <pebble.h>

#include "detail_window.h"
#include "../stocks.h"
#include "../layers/graph_layer.h"

static Window         *s_detail_window;
static StatusBarLayer *s_status_bar_layer;
static Layer          *s_debug_divider_layer;

/*
    -----------------------------
    |            |              |
    |      1     |       2      |
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

// 1 — symbol / price
static TextLayer *s_symbol_layer;
static TextLayer *s_price_layer;
static char       s_symbol_buffer[18];
static char       s_price_buffer[18];

// 2 — change % / change $
static TextLayer *s_change_percent_layer;
static TextLayer *s_change_layer;
static char       s_change_percent_buffer[18];
static char       s_change_buffer[18];

// 3 — graph
static GraphLayer *s_graph_layer;

// State
static StockData_t s_current_quote;

static const char *TIMEFRAMES[] = { "1D", "1W", "1M", "3M", "YTD", "1Y" };
#define TIMEFRAME_COUNT 6
static int s_timeframe_index = 1;  // default: "1W"

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

Window *detail_window_get_window(void) {
    return s_detail_window;
}

const char *detail_window_get_symbol(void) {
    return s_current_quote.symbol;
}

void detail_window_on_history_updated(void) {
    StockData_t *live = stocks_get_quote((int)s_current_quote.position);
    if (live) s_current_quote = *live;

    if (s_graph_layer && s_current_quote.history && s_current_quote.history->count > 0) {
        graph_layer_set_data(s_graph_layer,
                             s_current_quote.history->closes,
                             s_current_quote.history->count,
                             s_current_quote.history->timeframe);
        layer_mark_dirty(s_graph_layer);
    }
}

// -------------------------------------------------------------------------
// Debug divider (disabled by default)
// -------------------------------------------------------------------------

static void debug_divider_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds    = layer_get_bounds(layer);
    int   top_third = bounds.size.h / 3;
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(0, top_third), GPoint(bounds.size.w, top_third));
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2, 0), GPoint(bounds.size.w / 2, top_third));
}

// -------------------------------------------------------------------------
// Button handlers — UP/DOWN cycle timeframes
// -------------------------------------------------------------------------

// static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
//     s_timeframe_index = (s_timeframe_index + TIMEFRAME_COUNT - 1) % TIMEFRAME_COUNT;
//     stocks_request_history(s_current_quote.symbol, TIMEFRAMES[s_timeframe_index]);
// }

// static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
//     s_timeframe_index = (s_timeframe_index + 1) % TIMEFRAME_COUNT;
//     stocks_request_history(s_current_quote.symbol, TIMEFRAMES[s_timeframe_index]);
// }

static void click_config_provider(void *context) {
    // window_single_click_subscribe(BUTTON_ID_UP,   up_click_handler);
    // window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// -------------------------------------------------------------------------
// Window lifecycle
// -------------------------------------------------------------------------

static void detail_window_load(Window *window) {
    int y_offset = STATUS_BAR_LAYER_HEIGHT;
    int x_offset = STATUS_BAR_LAYER_HEIGHT / 2;

    Layer *window_layer  = window_get_root_layer(window);
    GRect  window_bounds = layer_get_bounds(window_layer);
    int    usable_h      = window_bounds.size.h - y_offset;
    int    top_section_h = usable_h / 3;

    // Status bar
    s_status_bar_layer = status_bar_layer_create();
#ifdef PBL_COLOR
    status_bar_layer_set_colors(s_status_bar_layer, GColorBlack, GColorWhite);
#else
    status_bar_layer_set_colors(s_status_bar_layer, GColorWhite, GColorBlack);
#endif
    status_bar_layer_set_separator_mode(s_status_bar_layer, StatusBarLayerSeparatorModeDotted);

    // ---- Segment 1: symbol (top half) + price (bottom half) ----
    int seg_w   = window_bounds.size.w / 2;
    int label_h = top_section_h / 2;

    GRect symbol_bounds = GRect(window_bounds.origin.x + x_offset,
                                window_bounds.origin.y + y_offset,
                                seg_w - x_offset,
                                label_h);
    s_symbol_layer = text_layer_create(symbol_bounds);
    snprintf(s_symbol_buffer, sizeof(s_symbol_buffer), "$%s", s_current_quote.symbol);
    text_layer_set_text(s_symbol_layer, s_symbol_buffer);
    text_layer_set_background_color(s_symbol_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_symbol_layer, GColorWhite);
#else
    text_layer_set_text_color(s_symbol_layer, GColorBlack);
#endif
    text_layer_set_text_alignment(s_symbol_layer, GTextAlignmentLeft);

    GRect price_bounds = GRect(window_bounds.origin.x + x_offset,
                               symbol_bounds.origin.y + label_h,
                               seg_w - x_offset,
                               top_section_h - label_h);
    s_price_layer = text_layer_create(price_bounds);
    snprintf(s_price_buffer, sizeof(s_price_buffer), "$%s", s_current_quote.price);
    text_layer_set_text(s_price_layer, s_price_buffer);
    text_layer_set_background_color(s_price_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_price_layer, GColorWhite);
#else
    text_layer_set_text_color(s_price_layer, GColorBlack);
#endif
    text_layer_set_text_alignment(s_price_layer, GTextAlignmentLeft);

    // ---- Segment 2: change % (top half) + change $ (bottom half) ----
    bool   is_negative  = s_current_quote.change[0] == '-';
    bool   is_zero      = s_current_quote.change[0] == '0';
#ifdef PBL_COLOR
    GColor change_color = is_negative ? GColorRed : (is_zero ? GColorLightGray : GColorGreen);
#else
    GColor change_color = GColorBlack;
#endif

    GRect change_pct_bounds = GRect(window_bounds.size.w / 2,
                                    window_bounds.origin.y + y_offset,
                                    seg_w - x_offset,
                                    label_h);
    s_change_percent_layer = text_layer_create(change_pct_bounds);
    snprintf(s_change_percent_buffer, sizeof(s_change_percent_buffer),
             "%s%%", s_current_quote.changePercent);
    text_layer_set_text(s_change_percent_layer, s_change_percent_buffer);
    text_layer_set_background_color(s_change_percent_layer, GColorClear);
    text_layer_set_text_color(s_change_percent_layer, change_color);
    text_layer_set_text_alignment(s_change_percent_layer, GTextAlignmentRight);

    GRect change_bounds = GRect(window_bounds.size.w / 2,
                                change_pct_bounds.origin.y + label_h,
                                seg_w - x_offset,
                                top_section_h - label_h);
    s_change_layer = text_layer_create(change_bounds);
    snprintf(s_change_buffer, sizeof(s_change_buffer), "%s", s_current_quote.change);
    text_layer_set_text(s_change_layer, s_change_buffer);
    text_layer_set_background_color(s_change_layer, GColorClear);
    text_layer_set_text_color(s_change_layer, change_color);
    text_layer_set_text_alignment(s_change_layer, GTextAlignmentRight);

    // ---- Segment 3: graph ----
    GRect graph_bounds = GRect(window_bounds.origin.x,
                               window_bounds.origin.y + y_offset + top_section_h,
                               window_bounds.size.w,
                               usable_h - top_section_h);
    s_graph_layer = graph_layer_create(graph_bounds);

    // Populate immediately if history is already cached for this symbol
    if (s_current_quote.history && s_current_quote.history->count > 0) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "History already cached for %s, populating graph immediately", s_current_quote.symbol);
        graph_layer_set_data(s_graph_layer,
                             s_current_quote.history->closes,
                             s_current_quote.history->count,
                             s_current_quote.history->timeframe);
    }

    // Fonts
#if PBL_PLATFORM_EMERY || PBL_PLATFORM_GABBRO
    text_layer_set_font(s_symbol_layer,        fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_font(s_price_layer,          fonts_get_system_font(FONT_KEY_GOTHIC_24));
    text_layer_set_font(s_change_percent_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_font(s_change_layer,         fonts_get_system_font(FONT_KEY_GOTHIC_24));
#else
    text_layer_set_font(s_symbol_layer,        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_font(s_price_layer,          fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_font(s_change_percent_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_font(s_change_layer,         fonts_get_system_font(FONT_KEY_GOTHIC_18));
#endif

    // Debug divider (keep off by default)
    GRect usable_bounds = GRect(window_bounds.origin.x,
                                window_bounds.origin.y + y_offset,
                                window_bounds.size.w,
                                usable_h);
    s_debug_divider_layer = layer_create(usable_bounds);
    layer_set_update_proc(s_debug_divider_layer, debug_divider_update_proc);

    layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_symbol_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_price_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_change_percent_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_change_layer));
    layer_add_child(window_layer, s_graph_layer);
    // layer_add_child(window_layer, s_debug_divider_layer);
}

static void detail_window_unload(Window *window) {
    layer_destroy(s_debug_divider_layer);
    status_bar_layer_destroy(s_status_bar_layer);
    text_layer_destroy(s_symbol_layer);
    text_layer_destroy(s_price_layer);
    text_layer_destroy(s_change_percent_layer);
    text_layer_destroy(s_change_layer);
    graph_layer_destroy(s_graph_layer);
    s_graph_layer = NULL;
}

void detail_window_deinit(void) {
    window_destroy(s_detail_window);
}

void detail_window_init(int position) {
    StockData_t *quote = stocks_get_quote(position);
    if (!quote) return;

    s_current_quote   = *quote;
    s_timeframe_index = 1;  // default: "1W"

    s_detail_window = window_create();
    window_set_click_config_provider(s_detail_window, click_config_provider);
    window_set_window_handlers(s_detail_window, (WindowHandlers) {
        .load   = detail_window_load,
        .unload = detail_window_unload,
    });

#ifdef PBL_COLOR
    window_set_background_color(s_detail_window, GColorBlack);
#else
    window_set_background_color(s_detail_window, GColorWhite);
#endif

    stocks_request_history(quote->symbol, TIMEFRAMES[s_timeframe_index]);

    window_stack_push(s_detail_window, true);
}
