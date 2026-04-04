#include <pebble.h>

#include "watchlist_window.h"
#include "detail_window.h"

static Window *s_watchlist_window;
static MenuLayer *s_tickers_layer;

static int s_watchlist_size = 0;
static StockData_t s_watchlist[10];
// static char s_last_updated[16];


static uint16_t get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 2;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    if (section_index == 0) return s_watchlist_size;
    return 0;
}

// static int16_t get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
//     if (section_index == 1) return 20;
//     return 0;
// }

// static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
//     if (section_index == 1) {
//         char buff[32];
//         snprintf(buff, sizeof(buff), "Last Updated: %s", s_last_updated);
// #ifdef PBL_COLOR
//         graphics_context_set_text_color(ctx, GColorLightGray);
// #else
//         graphics_context_set_text_color(ctx, GColorBlack);
// #endif
//         graphics_draw_text(ctx, buff,
//             fonts_get_system_font(FONT_KEY_GOTHIC_18),
//             layer_get_bounds(cell_layer),
//             GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
//     }
// }

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    char s_title_buff[16];
    char s_subtitle_buff[18];

    GRect bounds = layer_get_bounds(cell_layer);

    if (cell_index->row < s_watchlist_size) {
        StockData_t quote = s_watchlist[cell_index->row];

        snprintf(s_title_buff, sizeof(s_title_buff), "%s", quote.symbol);
        snprintf(s_subtitle_buff, sizeof(s_subtitle_buff), "$%s", quote.price);

#ifdef PBL_COLOR
        GColor bg_color;
        if (quote.change[0] != '-' && quote.change[0] != '0') {
            bg_color = menu_cell_layer_is_highlighted(cell_layer) ? GColorIslamicGreen : GColorBlack;
        } else if (quote.change[0] == '-') {
            bg_color = menu_cell_layer_is_highlighted(cell_layer) ? GColorDarkCandyAppleRed : GColorBlack;
        } else {
            bg_color = menu_cell_layer_is_highlighted(cell_layer) ? GColorLightGray : GColorBlack;
        }
        graphics_context_set_text_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, bg_color);
#endif

        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        menu_cell_basic_draw(ctx, cell_layer, s_title_buff, s_subtitle_buff, NULL);
    }
}

static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
#if PBL_PLATFORM_EMERY || PBL_PLATFORM_GABBRO
    return 53;
#else
    return 42;
#endif
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    detail_window_init(&s_watchlist[cell_index->row]);
}

static void watchlist_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_get_root_layer(window));

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
        .get_num_rows = get_num_rows_callback,
        // .get_header_height = get_header_height_callback,
        // .draw_header = draw_header_callback,
        .draw_row = draw_row_callback,
        .get_cell_height = get_cell_height_callback,
        .select_click = select_callback
    });

    layer_add_child(window_layer, menu_layer_get_layer(s_tickers_layer));
}

static void watchlist_window_unload(Window *window) {
    menu_layer_destroy(s_tickers_layer);
}

void watchlist_window_deinit(void) {
    window_destroy(s_watchlist_window);
}

void watchlist_window_init(StockData_t *quotes, int quote_count) {
    s_watchlist_size = quote_count;
    for (int i = 0; i < quote_count && i < 10; i++) {
        s_watchlist[i] = quotes[i];
    }
    // strncpy(s_last_updated, quotes[0].lastUpdated, sizeof(s_last_updated) - 1);

    s_watchlist_window = window_create();
    window_set_window_handlers(s_watchlist_window, (WindowHandlers) {
        .load = watchlist_window_load,
        .unload = watchlist_window_unload,
    });
#ifdef PBL_COLOR
    window_set_background_color(s_watchlist_window, GColorBlack);
#else
    window_set_background_color(s_watchlist_window, GColorWhite);
#endif
    window_stack_push(s_watchlist_window, true);
}
