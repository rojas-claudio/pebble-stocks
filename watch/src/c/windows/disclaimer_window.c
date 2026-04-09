#include <pebble.h>

#include "disclaimer_window.h"

#define PERSIST_KEY_DISCLAIMER_ACCEPTED 1

#define PADDING   4
#define HINT_H   20
#define TEXT_H  600  // generous fixed height — avoids calling graphics_text_layout_get_content_size outside a drawing context

static const char *DISCLAIMER_TEXT =
    "FOR INFORMATIONAL USE ONLY\n\n"
    "Data is delayed a minimum of 5 minutes and may be inaccurate or incomplete. "
    "Prices are sourced from third-party providers and are not guaranteed.\n\n"
    "NOT FINANCIAL ADVICE\n\n"
    "This app is for informational purposes only. Do not make investment or trading "
    "decisions based on data shown here. The developer assumes no liability for "
    "losses incurred from use of this app.\n\n"
    "Press SELECT to acknowledge.";

static Window      *s_disclaimer_window;
static ScrollLayer *s_scroll_layer;
static TextLayer   *s_text_layer;
static TextLayer   *s_hint_layer;

static void (*s_on_accepted)(void) = NULL;

// -------------------------------------------------------------------------
// Button handlers
// -------------------------------------------------------------------------

#define SCROLL_STEP 30

static int s_scroll_offset = 0;

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    s_scroll_offset -= SCROLL_STEP;
    if (s_scroll_offset < 0) s_scroll_offset = 0;
    scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, -s_scroll_offset), true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    GRect frame      = layer_get_frame(scroll_layer_get_layer(s_scroll_layer));
    int   max_offset = TEXT_H - frame.size.h;
    s_scroll_offset += SCROLL_STEP;
    if (s_scroll_offset > max_offset) s_scroll_offset = max_offset;
    scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, -s_scroll_offset), true);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    persist_write_bool(PERSIST_KEY_DISCLAIMER_ACCEPTED, true);
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
    Layer *window_layer = window_get_root_layer(window);
    GRect  bounds       = layer_get_bounds(window_layer);

    // Hint pinned to the bottom of the window, outside the scroll area
    s_hint_layer = text_layer_create(
        GRect(0, bounds.size.h - HINT_H, bounds.size.w, HINT_H));
    text_layer_set_text(s_hint_layer, "SELECT to acknowledge");
    text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
    text_layer_set_overflow_mode(s_hint_layer, GTextOverflowModeTrailingEllipsis);
    text_layer_set_background_color(s_hint_layer, GColorClear);
    text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
#ifdef PBL_COLOR
    text_layer_set_text_color(s_hint_layer, GColorLightGray);
#else
    text_layer_set_text_color(s_hint_layer, GColorBlack);
#endif

    // ScrollLayer fills everything above the hint
    int scroll_h = bounds.size.h - HINT_H;
    s_scroll_layer = scroll_layer_create(GRect(0, 0, bounds.size.w, scroll_h));
    scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, TEXT_H));
    scroll_layer_set_shadow_hidden(s_scroll_layer, true);

    // TextLayer lives inside the scroll layer
    s_text_layer = text_layer_create(
        GRect(PADDING, PADDING, bounds.size.w - 2 * PADDING, TEXT_H - PADDING));
    text_layer_set_text(s_text_layer, DISCLAIMER_TEXT);
    text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);
    text_layer_set_background_color(s_text_layer, GColorClear);
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
#ifdef PBL_COLOR
    text_layer_set_text_color(s_text_layer, GColorWhite);
#else
    text_layer_set_text_color(s_text_layer, GColorBlack);
#endif

    scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
    layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_hint_layer));
}

static void disclaimer_window_unload(Window *window) {
    text_layer_destroy(s_text_layer);
    text_layer_destroy(s_hint_layer);
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
