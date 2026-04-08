#include <pebble.h>

#include "disclaimer_window.h"

#define PERSIST_KEY_DISCLAIMER_ACCEPTED 1

static Window    *s_disclaimer_window;
static TextLayer *s_text_layer;
static TextLayer *s_hint_layer;

static void (*s_on_accepted)(void) = NULL;

// -------------------------------------------------------------------------
// Button handler
// -------------------------------------------------------------------------

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    persist_write_bool(PERSIST_KEY_DISCLAIMER_ACCEPTED, true);
    window_stack_remove(s_disclaimer_window, true);
    if (s_on_accepted) s_on_accepted();
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// -------------------------------------------------------------------------
// Window lifecycle
// -------------------------------------------------------------------------

static void disclaimer_window_load(Window *window) {
    Layer *window_layer  = window_get_root_layer(window);
    GRect  bounds        = layer_get_bounds(window_layer);

    // Main disclaimer text — leave room for the hint at the bottom
    GRect text_bounds = GRect(4, 4, bounds.size.w - 8, bounds.size.h - 28);
    s_text_layer = text_layer_create(text_bounds);
    text_layer_set_text(s_text_layer,
        "FOR INFORMATIONAL USE ONLY\n\n"
        "Data is delayed 5-15+ minutes and may be inaccurate or incomplete.\n\n"
        "Not financial advice. Do not trade based on this app.");
    text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);
    text_layer_set_background_color(s_text_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_text_layer, GColorWhite);
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
#else
    text_layer_set_text_color(s_text_layer, GColorBlack);
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
#endif

    // "Press SELECT" hint pinned to the bottom
    GRect hint_bounds = GRect(0, bounds.size.h - 24, bounds.size.w, 24);
    s_hint_layer = text_layer_create(hint_bounds);
    text_layer_set_text(s_hint_layer, "SELECT to acknowledge");
    text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
    text_layer_set_overflow_mode(s_hint_layer, GTextOverflowModeTrailingEllipsis);
    text_layer_set_background_color(s_hint_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_hint_layer, GColorLightGray);
#else
    text_layer_set_text_color(s_hint_layer, GColorBlack);
#endif
    text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

    layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_hint_layer));
}

static void disclaimer_window_unload(Window *window) {
    text_layer_destroy(s_text_layer);
    text_layer_destroy(s_hint_layer);
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
