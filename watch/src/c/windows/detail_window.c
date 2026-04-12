#include <pebble.h>

#include "detail_window.h"
#include "../stocks.h"
#include "../layers/graph_layer.h"
#include "../layers/info_layer.h"

typedef enum {
    Timeframe1DAction = 0,
    Timeframe1WAction,
    Timeframe1MAction,
    Timeframe3MAction,
    TimeframeYTDAction,
    Timeframe1YAction,
    RefreshAction
} ActionType;

// -------------------------------------------------------------------------
// Window
// -------------------------------------------------------------------------

static Window         *s_detail_window;
static InfoLayer      *s_info_layer;

// -------------------------------------------------------------------------
// ActionMenu
// -------------------------------------------------------------------------

static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level, *s_timeframes_level;
static ActionType s_current_type;

// -------------------------------------------------------------------------
// State
// -------------------------------------------------------------------------

static StockData_t s_current_quote;
static int s_timeframe_index_key = 0;
static int s_timeframe_index = 2;
static const char *TIMEFRAMES[] = { "1D", "1W", "1M", "3M", "YTD", "1Y" };


// -------------------------------------------------------------------------
// Debug
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// 1
// -------------------------------------------------------------------------

static TextLayer *s_symbol_layer;
static TextLayer *s_price_layer;
static char       s_symbol_buffer[18];
static char       s_price_buffer[18];

// -------------------------------------------------------------------------
// 2
// -------------------------------------------------------------------------

static TextLayer *s_change_percent_layer;
static TextLayer *s_change_layer;
static char       s_change_percent_buffer[18];
static char       s_change_buffer[18];

// -------------------------------------------------------------------------
// 3
// -------------------------------------------------------------------------

static GraphLayer *s_graph_layer;

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

Window *detail_window_get_window(void) {
    return s_detail_window;
}

const char *detail_window_get_symbol(void) {
    return s_current_quote.symbol;
}

static void refresh_history(void) {
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

static void refresh_quote(void) {
    StockData_t *live = stocks_get_quote((int)s_current_quote.position);
    if (!live) return;
    s_current_quote = *live;

    snprintf(s_price_buffer,          sizeof(s_price_buffer),         "$%s",  s_current_quote.price);
    snprintf(s_change_buffer,         sizeof(s_change_buffer),         "%s",   s_current_quote.change);
    snprintf(s_change_percent_buffer, sizeof(s_change_percent_buffer), "%s%%", s_current_quote.changePercent);

#if defined(PBL_COLOR)
    bool   is_negative  = s_current_quote.change[0] == '-';
    bool   is_zero      = s_current_quote.change[0] == '0';
    GColor change_color = PBL_IF_COLOR_ELSE(is_negative ? GColorRed : (is_zero ? GColorLightGray : GColorGreen), GColorBlack);
    text_layer_set_text_color(s_change_layer,         change_color);
    text_layer_set_text_color(s_change_percent_layer, change_color);
#endif

    layer_mark_dirty(text_layer_get_layer(s_price_layer));
    layer_mark_dirty(text_layer_get_layer(s_change_layer));
    layer_mark_dirty(text_layer_get_layer(s_change_percent_layer));
}

static void on_quote_updated(int position) {
    if (position != (int)s_current_quote.position) return;
    refresh_quote();
}

static void on_history_updated(int position) {
    if (position != (int)s_current_quote.position) return;
    refresh_history();
}

// -------------------------------------------------------------------------
// Debug divider (disabled by default)
// -------------------------------------------------------------------------

static void debug_divider_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds    = layer_get_bounds(layer);
#if defined(PBL_PLATFORM_GABBRO)
    int   top       = bounds.size.h / 4;
#else
    int   top       = bounds.size.h / 3;
#endif
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(0, top), GPoint(bounds.size.w, top));
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2, 0), GPoint(bounds.size.w / 2, top));
}

// -------------------------------------------------------------------------
// ActionMenu
// -------------------------------------------------------------------------

static void action_performed_callback(ActionMenu *menu, const ActionMenuItem *action,
                                                        void *context) {
    s_current_type = (ActionType)action_menu_item_get_action_data(action);

    if (s_current_type == RefreshAction) {
        stocks_request_refresh();
    } else {
        s_timeframe_index = (int)s_current_type;

        persist_write_int(s_timeframe_index_key, s_timeframe_index);

        stocks_request_history(s_current_quote.symbol, TIMEFRAMES[s_timeframe_index]);
    }
}

static void init_action_menu(void) {
    s_root_level = action_menu_level_create(2);

    action_menu_level_add_action(s_root_level, "Refresh", action_performed_callback, (void *)RefreshAction);

    s_timeframes_level = action_menu_level_create(TIMEFRAME_COUNT);
    action_menu_level_add_child(s_root_level, s_timeframes_level, "Graph");

    for (int i = 0; i < TIMEFRAME_COUNT; i++) {
        action_menu_level_add_action(s_timeframes_level, TIMEFRAMES[i], action_performed_callback, (void *)i);
    }
}

// -------------------------------------------------------------------------
// Button handlers — UP/DOWN cycle timeframes
// -------------------------------------------------------------------------

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    bool is_up = s_current_quote.change[0] != '-';

    ActionMenuConfig config = (ActionMenuConfig) {
        .root_level = s_root_level,
        .colors = {
            .background = PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite),
            .foreground = PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack),
        },
        .align = ActionMenuAlignCenter
    };

    if (is_up) {
        config.colors.background = GColorGreen;
        config.colors.foreground = GColorWhite;
    } else {
        config.colors.background = GColorRed;
        config.colors.foreground = GColorWhite;
    }
    
    s_action_menu = action_menu_open(&config);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// -------------------------------------------------------------------------
// Tick handler — updates the info bar every minute
// -------------------------------------------------------------------------

static void update_time() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    info_layer_set_data(s_info_layer, tick_time, s_current_quote.marketHours, s_current_quote.position + 1, s_current_quote.size);
    layer_mark_dirty(s_info_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

// -------------------------------------------------------------------------
// Window lifecycle
// -------------------------------------------------------------------------

static void detail_window_appear(Window *window) {
    stocks_on_quote_updated(on_quote_updated);
    stocks_on_history_updated(on_history_updated);
}

static void detail_window_disappear(Window *window) {
    stocks_on_quote_updated(NULL);
    stocks_on_history_updated(NULL);
}

static void detail_window_load(Window *window) {
    Layer *window_layer  = window_get_root_layer(window);
    GRect  window_bounds = layer_get_bounds(window_layer);

    // -------------------------------------------------------------------------
    // Layout constants
    // -------------------------------------------------------------------------

    int y_offset      = STATUS_BAR_LAYER_HEIGHT;
    int x_offset      = STATUS_BAR_LAYER_HEIGHT / 2;

    int usable_h      = window_bounds.size.h - y_offset;

    int top_section_h = usable_h / 3;
    int seg_w         = window_bounds.size.w / 2;
    int label_h       = top_section_h / 2;

#if defined(PBL_PLATFORM_GABBRO)
    top_section_h = usable_h / 4;
    label_h       = top_section_h / 2;
#endif

    // -------------------------------------------------------------------------
    // Bounds
    // -------------------------------------------------------------------------

    GRect symbol_bounds = GRect(window_bounds.origin.x + x_offset,
                                window_bounds.origin.y + y_offset,
                                seg_w - x_offset,
                                label_h);
    GRect price_bounds = GRect(window_bounds.origin.x + x_offset,
                               symbol_bounds.origin.y + label_h,
                               seg_w - x_offset,
                               top_section_h - label_h);
    GRect change_pct_bounds = GRect(seg_w,
                                    window_bounds.origin.y + y_offset,
                                    seg_w - x_offset,
                                    label_h);
    GRect change_bounds = GRect(seg_w,
                                change_pct_bounds.origin.y + label_h,
                                seg_w - x_offset,
                                top_section_h - label_h);
    GRect graph_bounds = GRect(window_bounds.origin.x,
                               window_bounds.origin.y + y_offset + top_section_h,
                               window_bounds.size.w,
                               usable_h - top_section_h);
    GRect debug_bounds = GRect(window_bounds.origin.x,
                               window_bounds.origin.y + y_offset,
                               window_bounds.size.w,
                               usable_h);

#if defined(PBL_ROUND)
    symbol_bounds.size.w   = window_bounds.size.w;
    symbol_bounds.size.h   = top_section_h / 2;
    symbol_bounds.origin.x = 0;

    price_bounds.origin.y = symbol_bounds.origin.y + symbol_bounds.size.h;
    price_bounds.size.w    = seg_w - x_offset;

    change_bounds.size.w   = seg_w - x_offset;
    change_bounds.origin.x = seg_w;
#endif

#if defined(PBL_PLATFORM_GABBRO)
    graph_bounds.size.w = 184;
    graph_bounds.size.h = 222 - top_section_h - y_offset;
    graph_bounds.origin.x = 38;
    graph_bounds.origin.y = 222 - (graph_bounds.size.h);
#elif defined(PBL_PLATFORM_CHALK)
    graph_bounds.size.w = 128;
    graph_bounds.size.h = 154 - top_section_h - y_offset;
    graph_bounds.origin.x = 26;
    graph_bounds.origin.y = 154 - (graph_bounds.size.h);
#endif

    // -------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------

#if defined(PBL_COLOR)
    bool   is_negative  = s_current_quote.change[0] == '-';
    bool   is_zero      = s_current_quote.change[0] == '0';
    GColor change_color = is_negative ? GColorRed : (is_zero ? GColorLightGray : GColorGreen);
#else
    GColor change_color = GColorBlack;
#endif

    // -------------------------------------------------------------------------
    // Fonts
    // -------------------------------------------------------------------------

#if PBL_PLATFORM_EMERY || PBL_PLATFORM_GABBRO
    GFont font_large = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    GFont font_small = fonts_get_system_font(FONT_KEY_GOTHIC_24);
#else
    GFont font_large = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    GFont font_small = fonts_get_system_font(FONT_KEY_GOTHIC_18);
#endif

    // -------------------------------------------------------------------------
    // Info bar
    // -------------------------------------------------------------------------

    s_info_layer = info_layer_create(GRect(window_bounds.origin.x, window_bounds.origin.y, window_bounds.size.w, STATUS_BAR_LAYER_HEIGHT));

    // -------------------------------------------------------------------------
    // Segment 1 — symbol + price
    // -------------------------------------------------------------------------

    s_symbol_layer = text_layer_create(symbol_bounds);
    snprintf(s_symbol_buffer, sizeof(s_symbol_buffer), "%s", s_current_quote.symbol);
    text_layer_set_text(s_symbol_layer, s_symbol_buffer);
    text_layer_set_background_color(s_symbol_layer, GColorClear);
    text_layer_set_text_color(s_symbol_layer, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
    text_layer_set_text_alignment(s_symbol_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
    text_layer_set_font(s_symbol_layer, font_large);

    s_price_layer = text_layer_create(price_bounds);
    snprintf(s_price_buffer, sizeof(s_price_buffer), "$%s", s_current_quote.price);
    text_layer_set_text(s_price_layer, s_price_buffer);
    text_layer_set_background_color(s_price_layer, GColorClear);
    text_layer_set_text_color(s_price_layer, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
    text_layer_set_text_alignment(s_price_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentRight));
    text_layer_set_font(s_price_layer, font_small);

    // -------------------------------------------------------------------------
    // Segment 2 — change % + change $
    // -------------------------------------------------------------------------

    s_change_percent_layer = text_layer_create(change_pct_bounds);
    snprintf(s_change_percent_buffer, sizeof(s_change_percent_buffer), "%s%%", s_current_quote.changePercent);
    text_layer_set_text(s_change_percent_layer, s_change_percent_buffer);
    text_layer_set_background_color(s_change_percent_layer, GColorClear);
    text_layer_set_text_color(s_change_percent_layer, change_color);
    text_layer_set_text_alignment(s_change_percent_layer, PBL_IF_RECT_ELSE(GTextAlignmentRight, GTextAlignmentLeft));
    text_layer_set_font(s_change_percent_layer, font_large);

    s_change_layer = text_layer_create(change_bounds);
    snprintf(s_change_buffer, sizeof(s_change_buffer), "%s", s_current_quote.change);
    text_layer_set_text(s_change_layer, s_change_buffer);
    text_layer_set_background_color(s_change_layer, GColorClear);
    text_layer_set_text_color(s_change_layer, change_color);
    text_layer_set_text_alignment(s_change_layer, PBL_IF_RECT_ELSE(GTextAlignmentRight, GTextAlignmentLeft));
    text_layer_set_font(s_change_layer, font_small);

    // -------------------------------------------------------------------------
    // Segment 3 — graph
    // -------------------------------------------------------------------------

    s_graph_layer = graph_layer_create(graph_bounds);
    if (s_current_quote.history && s_current_quote.history->count > 0) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "History cached for %s, populating graph", s_current_quote.symbol);
        graph_layer_set_data(s_graph_layer,
                             s_current_quote.history->closes,
                             s_current_quote.history->count,
                             s_current_quote.history->timeframe);
    }

    // -------------------------------------------------------------------------
    // Debug divider (disabled by default)
    // -------------------------------------------------------------------------

    s_debug_divider_layer = layer_create(debug_bounds);
    layer_set_update_proc(s_debug_divider_layer, debug_divider_update_proc);

    // -------------------------------------------------------------------------
    // Layer hierarchy
    // -------------------------------------------------------------------------

    layer_add_child(window_layer, s_info_layer);
    layer_add_child(window_layer, text_layer_get_layer(s_symbol_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_price_layer));
#if defined(PBL_RECT)
    layer_add_child(window_layer, text_layer_get_layer(s_change_percent_layer));
#endif
    layer_add_child(window_layer, text_layer_get_layer(s_change_layer));
    layer_add_child(window_layer, s_graph_layer);
    // layer_add_child(window_layer, s_debug_divider_layer);

    update_time();
}

static void detail_window_unload(Window *window) {
    tick_timer_service_unsubscribe();
    layer_destroy(s_debug_divider_layer);
    info_layer_destroy(s_info_layer);
    s_info_layer = NULL;
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
    s_timeframe_index = 2; // default to 1M
    s_timeframe_index = persist_read_int(s_timeframe_index_key);

    s_detail_window = window_create();
    window_set_click_config_provider(s_detail_window, click_config_provider);
    window_set_window_handlers(s_detail_window, (WindowHandlers) {
        .load      = detail_window_load,
        .unload    = detail_window_unload,
        .appear    = detail_window_appear,
        .disappear = detail_window_disappear,
    });
    window_set_background_color(s_detail_window, PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite));

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    stocks_request_history(quote->symbol, TIMEFRAMES[s_timeframe_index]);

    window_stack_push(s_detail_window, true);

    init_action_menu();
}
