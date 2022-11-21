#include <pebble.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TICKER_LENGTH 5
#define MAX_TICKERS 15
#define MAX_TICKER_DATA 10
#define MAX_HIS_DATA 140
#define TICKER_CELL_MIN_HEIGHT PBL_IF_ROUND_ELSE(49, 45)

static Window *s_window;
static Window *s_detail_window;
static TextLayer *s_text_layer;
static MenuLayer *s_menu_layer;
static Layer *s_canvas_layer;
static Layer *s_detail_layer;

static int received = 0;
static int total = -1;

static int selected;

//watchlist data
static char tickers[MAX_TICKERS][MAX_TICKER_LENGTH]; 
static char open[MAX_TICKERS][MAX_TICKER_DATA]; 
static char high[MAX_TICKERS][MAX_TICKER_DATA]; 
static char low[MAX_TICKERS][MAX_TICKER_DATA]; 
static char price[MAX_TICKERS][MAX_TICKER_DATA];
static char close[MAX_TICKERS][MAX_TICKER_DATA]; 
static char change[MAX_TICKERS][MAX_TICKER_DATA];
static int32_t change_int[MAX_TICKERS]; 
static char changePercent[MAX_TICKERS][MAX_TICKER_DATA]; 

static uint8_t close_history[MAX_TICKERS][MAX_HIS_DATA]; 

static void select_click_callback(MenuLayer *s_menu_layer, MenuIndex *cell_index, void *data) {
  //if (received != 0) {
    selected = cell_index->row;
    window_stack_push(s_detail_window, true);
  //}
  
}

static void s_detail_layer_update_proc(Layer *layer, GContext *ctx) {
  int id = total - (selected - 1) - 2;
  char buffer[16];

  strcpy(buffer, price[id]);
  strcat(buffer, " (");
  strcat(buffer, change[id]);
  strcat(buffer, ")");

  GRect bounds = layer_get_bounds(layer);
  GFont gothic_large = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  GFont gothic_small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  graphics_context_set_text_color(ctx, GColorWhite);

#if(!PBL_ROUND)
  graphics_draw_text(ctx, tickers[id], gothic_large, GRect(5, -3, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, buffer, gothic_small, GRect(5, 27, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, changePercent[id], gothic_small, GRect(5, 42, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
#endif
#if(PBL_ROUND)
  graphics_draw_text(ctx, tickers[id], gothic_large, GRect(0, 7, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, price[id], gothic_small, GRect(0, 40, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, changePercent[id], gothic_small, GRect(0, 60, bounds.size.w, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#endif 

#if(!PBL_ROUND)
  int offset = 3;
#endif
#if(PBL_ROUND)
  int offset = 22;
  int y_offset = -10;
#endif
  for (int i = 0; i < 139; i++) {
    graphics_context_set_antialiased(ctx, true);
    graphics_context_set_stroke_width(ctx, 1.5);
    graphics_context_set_stroke_color(ctx, GColorWhite);

#if(!PBL_ROUND)
    graphics_draw_line(ctx, GPoint(bounds.origin.x + i + offset, (bounds.size.h / 3) + (close_history[id][i]-10)), GPoint(bounds.origin.x + i + 1 + offset, (bounds.size.h / 3) + (close_history[id][i + 1]-10)));
#endif
#if(PBL_ROUND)
    graphics_draw_line(ctx, GPoint(bounds.origin.x + i + offset, (bounds.size.h / 3) + (close_history[id][i]-10) + y_offset), GPoint(bounds.origin.x + i + 1 + offset, (bounds.size.h / 3) + (close_history[id][i + 1]-10) + y_offset));
#endif
  }
}

static void detail_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_detail_layer = layer_create(bounds);

  window_set_background_color(window, GColorBlack);

  layer_set_update_proc(s_detail_layer, s_detail_layer_update_proc);
  layer_add_child(window_layer, s_detail_layer);
}

static uint16_t get_num_rows_callback(MenuLayer *s_menu_layer, uint16_t section_index, void *context) {
  return total;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return TICKER_CELL_MIN_HEIGHT;
}

static void draw_ticker_cell(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index) {
  int id = total - (cell_index->row - 1) - 2;

  GRect bounds = layer_get_bounds(cell_layer);
  GRect text_bounds;
  GTextAlignment alignment = GTextAlignmentLeft;

  graphics_draw_rect(ctx, bounds);

  if (change_int[id] > 0) { //convert change to int and check if it's positive or negative
    if(menu_cell_layer_is_highlighted(cell_layer)) { //if positive, set highlight color to green
      graphics_context_set_fill_color(ctx, GColorGreen);
    } else {
      graphics_context_set_fill_color(ctx, GColorBlack);
    }
  } else if (change_int[id] < 0) { //if negative, set highlight color to red
    if(menu_cell_layer_is_highlighted(cell_layer)) {
      graphics_context_set_fill_color(ctx, GColorRed);
    } else {
      graphics_context_set_fill_color(ctx, GColorBlack);
    }
  } else {
    if(menu_cell_layer_is_highlighted(cell_layer)) { //if no change, set highlight color to gray
      graphics_context_set_fill_color(ctx, GColorLightGray);
    } else {
      graphics_context_set_fill_color(ctx, GColorBlack);
    } 
  }

  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if(bounds.size.h == TICKER_CELL_MIN_HEIGHT) {
    menu_cell_basic_draw(ctx, cell_layer, tickers[id], changePercent[id], NULL);
  }

  if (menu_cell_layer_is_highlighted(cell_layer)) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }

  received = 0; //once everything has been drawn, reset the counter
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  draw_ticker_cell(ctx, cell_layer, cell_index);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message received!");

  Tuple *total_tuple = dict_find (iter, MESSAGE_KEY_TotalTickers);
  if (total_tuple) {
    total = total_tuple->value->int32;
  }

  Tuple *symbol_tuple = dict_find(iter, MESSAGE_KEY_Symbol);
  if (symbol_tuple) {
    strcpy(tickers[received], symbol_tuple->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Symbol %s received", tickers[received]);
  }

  Tuple *open_tuple = dict_find(iter, MESSAGE_KEY_Open);
  if (open_tuple) {
    strcpy(open[received], open_tuple->value->cstring);
  }
  
  Tuple *high_tuple = dict_find(iter, MESSAGE_KEY_High);
  if (high_tuple) {
    strcpy(high[received], high_tuple->value->cstring);
  }
  
  Tuple *low_tuple = dict_find(iter, MESSAGE_KEY_Low);
  if (low_tuple) {
    strcpy(low[received], low_tuple->value->cstring);
  }

  Tuple *price_tuple = dict_find(iter, MESSAGE_KEY_Price);
  if (price_tuple) {
    strcpy(price[received], price_tuple->value->cstring);
  }

  Tuple *close_tuple = dict_find(iter, MESSAGE_KEY_PrevClose);
  if (close_tuple) {
    strcpy(close[received], close_tuple->value->cstring);
  }
  
  Tuple *close_history_tuple = dict_find(iter, MESSAGE_KEY_CloseHistory);
  if (close_history_tuple) {
    memcpy(close_history[received], close_history_tuple->value->data, close_history_tuple->length);
    persist_write_data(MESSAGE_KEY_CloseHistory, close_history, sizeof(close_history));
  }
  
  Tuple *change_tuple = dict_find(iter, MESSAGE_KEY_Change);
  if (change_tuple) {
    strcpy(change[received], change_tuple->value->cstring);
  }

  Tuple *change_int_tuple = dict_find(iter, MESSAGE_KEY_ChangeInt);
  if (change_int_tuple) {
    change_int[received] = change_int_tuple->value->int32;
  }

  Tuple *change_percent_tuple = dict_find(iter, MESSAGE_KEY_ChangePercent);
  if (change_percent_tuple) {
    strcpy(changePercent[received], change_percent_tuple->value->cstring);
  }
  
  received++;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d of %d", received, total);
  if (received == total) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading menu layer");
    layer_remove_from_parent(text_layer_get_layer(s_text_layer));
    layer_add_child(window_get_root_layer(s_window), menu_layer_get_layer(s_menu_layer));
    menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
    menu_layer_reload_data(s_menu_layer);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context){
  //handle failed message
  APP_LOG(APP_LOG_LEVEL_DEBUG, "dropped message %d", 0);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_menu_layer = menu_layer_create(bounds);

#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
#endif
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_click_callback,
  });

  if (received == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing no data message");
    //create a text layer at the origin and span the entire screen
    s_text_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
    text_layer_set_text(s_text_layer, "\nLoading...\n\n\nMake sure you have added tickers to your watchlist in the Pebble app.");
    text_layer_set_background_color(s_text_layer, GColorBlack);
    text_layer_set_text_color(s_text_layer, GColorWhite);
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_text_layer));
  }
}

static void window_unload() {
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_text_layer);
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_window);
  window_destroy(s_detail_window);
} 

static void init() {
  s_window = window_create();
  s_detail_window = window_create();
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_set_window_handlers(s_detail_window, (WindowHandlers) {
      .load = detail_window_load,
  });

  window_stack_push(s_window, true);
}

static void deinit() {
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}