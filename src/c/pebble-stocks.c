/*
  TO-DO BEFORE HACKATHON:
    - Add a "refresh" button to the main screen
    - Display detailed stock data upon clicking a ticker
    - Figure out data being sent out of order (relative to array in index.js)
  TO-DO (NO DEADLINE):
    - Implement graphs for tickers
*/
#include <pebble.h>
#include <math.h>
#include <string.h>

#define MAX_TICKER_LENGTH 5
#define TICKER_CELL_MIN_HEIGHT PBL_IF_ROUND_ELSE(49, 45)

typedef struct TickerItem {
  // the name displayed for the item
  char name[MAX_TICKER_LENGTH];
} TickerItem;

static Window *s_window;
static Window *s_detail_window;
static TextLayer *s_text_layer;
static MenuLayer *s_menu_layer;
static Layer *s_canvas_layer;

static TickerItem *s_ticker_items[10];

static int received = 0;
static int total = -1;

static int selected;

//watchlist data
static char tickers[15][MAX_TICKER_LENGTH]; 
static char open[15][10]; 
static char high[15][10]; 
static char low[15][10]; 
static char price[15][10];
static char close[15][10]; 
static char change[15][10]; 
static char changePercent[15][10]; 

//create an three dimensional array of strings to hold the data
static char close_week[15][10][10];

static void select_click_callback(MenuLayer *s_menu_layer, MenuIndex *cell_index, void *data) {
  // Get which row
  selected = cell_index->row;
  window_stack_push(s_detail_window, true);
}

static void detail_window_load(Window *window) {
  int id = total - (selected - 1) - 2;
  // char buffer[16];

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD); Draw text using graphics_draw_text
  // graphics_context_set_text_color(ctx, GColorWhite);


  TextLayer *ticker_label = text_layer_create(GRect(5, -3, bounds.size.w, bounds.size.h));
  TextLayer *price_label = text_layer_create(GRect(0, 40, bounds.size.w, bounds.size.h));
  TextLayer *change_label = text_layer_create(GRect(0, 60, bounds.size.w, bounds.size.h));
#if(PBL_ROUND)
  ticker_label = text_layer_create(GRect(0, 7, bounds.size.w, bounds.size.h));
#endif

  text_layer_set_text(ticker_label, tickers[id]);
  text_layer_set_font(ticker_label, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(ticker_label, GTextAlignmentLeft);
  text_layer_set_background_color(ticker_label, GColorBlack);
  text_layer_set_text_color(ticker_label, GColorWhite);
#if(PBL_ROUND)
  text_layer_set_text_alignment(ticker_label, GTextAlignmentCenter);
#endif

  text_layer_set_text(price_label, price[id]);
  text_layer_set_font(price_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(price_label, GColorBlack);
  text_layer_set_text_color(price_label, GColorWhite);
  text_layer_set_text_alignment(price_label, GTextAlignmentLeft);
#if(PBL_ROUND)
  text_layer_set_text_alignment(price_label, GTextAlignmentCenter);
#endif

  text_layer_set_text(change_label, changePercent[id]);
  text_layer_set_font(change_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(change_label, GColorBlack);
  text_layer_set_text_color(change_label, GColorWhite);
  text_layer_set_text_alignment(change_label, GTextAlignmentLeft);
#if(PBL_ROUND)
  text_layer_set_text_alignment(change_label, GTextAlignmentCenter);
#endif

  layer_add_child(window_get_root_layer(s_detail_window), text_layer_get_layer(ticker_label));
  layer_add_child(window_get_root_layer(s_detail_window), text_layer_get_layer(price_label));
  layer_add_child(window_get_root_layer(s_detail_window), text_layer_get_layer(change_label));

  window_set_background_color(s_detail_window, GColorBlack);
}

static uint16_t get_num_rows_callback(MenuLayer *s_menu_layer, uint16_t section_index, void *context) {
  return received;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return TICKER_CELL_MIN_HEIGHT;
}

static void draw_ticker_cell(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index) {
  int id = total - (cell_index->row - 1) - 2;

  TickerItem *item = s_ticker_items[id];

  GRect bounds = layer_get_bounds(cell_layer);
  GRect text_bounds;
  GTextAlignment alignment = GTextAlignmentLeft;

  graphics_draw_rect(ctx, bounds);
  if (atoi(change[id]) > 0) { //convert change to int and check if it's positive or negative
    if(menu_cell_layer_is_highlighted(cell_layer)) { //if positive, set highlight color to green
      graphics_context_set_fill_color(ctx, GColorGreen);
    } else {
      graphics_context_set_fill_color(ctx, GColorBlack);
    }
  } else if (atoi(change[id]) < 0) { //if negative, set highlight color to red
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
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  draw_ticker_cell(ctx, cell_layer, cell_index);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message received!");
  
  Tuple *symbol_tuple = dict_find(iter, MESSAGE_KEY_Symbol);
  Tuple *open_tuple = dict_find(iter, MESSAGE_KEY_Open);
  Tuple *high_tuple = dict_find(iter, MESSAGE_KEY_High);
  Tuple *low_tuple = dict_find(iter, MESSAGE_KEY_Low);
  Tuple *price_tuple = dict_find(iter, MESSAGE_KEY_Price);
  Tuple *close_tuple = dict_find(iter, MESSAGE_KEY_PrevClose);
  Tuple *close_history_tuple = dict_find(iter, MESSAGE_KEY_CloseHistory);
  Tuple *change_tuple = dict_find(iter, MESSAGE_KEY_Change);
  Tuple *change_percent_tuple = dict_find(iter, MESSAGE_KEY_ChangePercent);
  Tuple *total_tuple = dict_find (iter, MESSAGE_KEY_TotalTickers);

  strcpy(tickers[received], symbol_tuple->value->cstring);
  strcpy(open[received], open_tuple->value->cstring);
  strcpy(high[received], high_tuple->value->cstring);
  strcpy(low[received], low_tuple->value->cstring);
  strcpy(price[received], price_tuple->value->cstring);
  strcpy(close[received], close_tuple->value->cstring);
  // strcpy(close_week[received], close_history_tuple->value->cstring);
  strcpy(change[received], change_tuple->value->cstring);
  strcpy(changePercent[received], change_percent_tuple->value->cstring);
  total = total_tuple->value->int32;

  received++;
  if (received > total) {
    received = 1; //restart counter once new batch of data is received
  }
  if (received == total) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading menu layer");
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

  // Loading text
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading...");
  // s_text_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  // text_layer_set_text(s_text_layer, "Loading...");
  // text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  // text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  // layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_text_layer));
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
#endif
  s_detail_window = window_create();
  window_set_window_handlers(s_detail_window, (WindowHandlers) {
      .load = detail_window_load,
  });

  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_click_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // request_data();
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
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
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