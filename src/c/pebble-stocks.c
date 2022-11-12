/*
  TO-DO BEFORE HACKATHON:
    - Add a "refresh" button to the main screen
    - Change highlight colours based on positive/negative change
    - Display detailed stock data upon clicking a ticker
    - Figure out data being sent out of order (relative to array in index.js)
  TO-DO (NO DEADLINE):
    - Implement graphs for tickers
*/

#include <pebble.h>
#include <math.h>

static Window *s_window;
static TextLayer *s_text_layer;
static MenuLayer *s_menu_layer;
static Layer *s_canvas_layer;

static int received = 0;
static int total = 0;

static char tickers[15][5]; 
static char open[15][10]; 
static char high[15][10]; 
static char low[15][10]; 
static char price[15][10]; 
static char volume[15][10]; 
static char close[15][10]; 
static char change[15][10]; 
static char changePercent[15][10]; 

static uint16_t get_num_rows_callback(MenuLayer *s_menu_layer, uint16_t section_index, void *context) {
  return received;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  menu_cell_basic_draw(ctx, cell_layer, tickers[cell_index->row], price[cell_index->row], NULL);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *symbol_tuple = dict_find(iter, MESSAGE_KEY_Symbol);
  Tuple *open_tuple = dict_find(iter, MESSAGE_KEY_Open);
  Tuple *high_tuple = dict_find(iter, MESSAGE_KEY_High);
  Tuple *low_tuple = dict_find(iter, MESSAGE_KEY_Low);
  Tuple *price_tuple = dict_find(iter, MESSAGE_KEY_Price);
  Tuple *volume_tuple = dict_find(iter, MESSAGE_KEY_Volume);
  Tuple *close_tuple = dict_find(iter, MESSAGE_KEY_PrevClose);
  Tuple *change_tuple = dict_find(iter, MESSAGE_KEY_Change);
  Tuple *change_percent_tuple = dict_find(iter, MESSAGE_KEY_ChangePercent);
  Tuple *ready_tuple = dict_find (iter, MESSAGE_KEY_Ready);
  Tuple *total_tuple = dict_find (iter, MESSAGE_KEY_TotalTickers);

  strcpy(tickers[received], symbol_tuple->value->cstring);
  strcpy(open[received], open_tuple->value->cstring);
  strcpy(high[received], high_tuple->value->cstring);
  strcpy(low[received], low_tuple->value->cstring);
  strcpy(price[received], price_tuple->value->cstring);
  strcpy(volume[received], volume_tuple->value->cstring);
  strcpy(close[received], close_tuple->value->cstring);
  strcpy(change[received], change_tuple->value->cstring);
  strcpy(changePercent[received], change_percent_tuple->value->cstring);
  total = total_tuple->value->int32;

  if (ready_tuple->value->int32 == 1) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Valid ticker data. Got ready tuple. %d", 1);
    received++;
  } else if (ready_tuple->value->int32 == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Not valid ticker data %d", 1);
  }

  if (received == total) { //only reload once all data has been received
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
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorGreen, GColorWhite);
#endif
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
    });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload() {
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_text_layer);
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_window);
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