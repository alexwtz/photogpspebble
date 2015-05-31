/*
 * main.c
 * Sets up Window, AppMessage and a TextLayer to show the message received.
 */

#include <pebble.h>

#define KEY_DATA 5
#define TAKE_GPS_LOCATION 25
#define RESTART_SESSION 42

static Window *s_main_window;
static TextLayer *s_output_layer;
static ActionBarLayer *s_action_bar_layer;
static BitmapLayer *s_black_layer;

static GBitmap *my_icon_location, *my_icon_stop, *back_icon;

static void send_int(int key, int value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &value, sizeof(int), true);
  app_message_outbox_send();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
   send_int(KEY_DATA,TAKE_GPS_LOCATION);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
   send_int(KEY_DATA,RESTART_SESSION);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while (t != NULL) {
    // Long lived buffer
    static char s_buffer[64];

    // Process this pair's key
    switch (t->key) {
      case KEY_DATA:
        // Copy value and display
        snprintf(s_buffer, sizeof(s_buffer), " %s", t->value->cstring);
        text_layer_set_text(s_output_layer, s_buffer);
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

//Config des bouttons
static void config_provider(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  //*********************************************************************************
   GRect bounds = layer_get_bounds(window_layer);

  // We do this to account for the offset due to the status bar
  // at the top of the app window.

  GPoint center = grect_center_point(&bounds);
#ifdef PBL_PLATFORM_BASALT
  GSize image_size = gbitmap_get_bounds(back_icon).size;
#else
  GSize image_size = back_icon->bounds.size;
#endif
  GRect image_frame = GRect(center.x-8, center.y-10, image_size.w, image_size.h);
  image_frame.origin.x -= image_size.w / 2;
  image_frame.origin.y -= image_size.h / 2;
  
  // Use GCompOpClear to display the black portions of the image
  s_black_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_bitmap(s_black_layer, back_icon);
  bitmap_layer_set_compositing_mode(s_black_layer, GCompOpClear);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_black_layer));
  //**************************************************************************************************
  
  // Create ActionBar
  // Initialize the action bar:
  s_action_bar_layer = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(s_action_bar_layer, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(s_action_bar_layer, config_provider);
  // Set the icons:
  // The loading the icons is omitted for brevity... See HeapBitmap.
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, my_icon_location);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, my_icon_stop);
  
    
  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(8, 130, 110, 25));
  //text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_output_layer, " Save pos");
  text_layer_set_text_color(s_output_layer,GColorWhite);
  text_layer_set_background_color(s_output_layer,GColorBlack);
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
  
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
  
  action_bar_layer_destroy(s_action_bar_layer);
}

static void init() {
  //create bitmaps
  my_icon_location = gbitmap_create_with_resource(RESOURCE_ID_LOCATION_BLACK);
  my_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_RESTART_BLACK);
  back_icon = gbitmap_create_with_resource(RESOURCE_ID_CAMERA120_BLACK);
    
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
  
  gbitmap_destroy(my_icon_location);
  gbitmap_destroy(my_icon_stop);
  gbitmap_destroy(back_icon);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
