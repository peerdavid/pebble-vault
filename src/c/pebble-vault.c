#include <pebble.h>

// Structure to hold text drawing parameters
typedef struct {
  char text[32];
  GFont font;
  GTextAlignment alignment;
} OutlinedText;

static Window *s_window;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static Layer *s_metal_effect_layer;
static Layer *s_time_layer;
static Layer *s_date_layer;
static Layer *s_vault_tec_layer;
static Layer *s_tagline_layer;
static Layer *s_steps_value_layer;
static Layer *s_steps_label_layer;
static Layer *s_battery_value_layer;
static Layer *s_battery_label_layer;

static OutlinedText s_time_text;
static OutlinedText s_date_text;

static char s_date_line1[16];
static char s_date_line2[16];
static char s_steps_line1[16];
static char s_steps_line2[16];
static char s_battery_line1[16];
static char s_battery_line2[16];

// Custom drawing function for text with outline
static void draw_outlined_text(GContext *ctx, OutlinedText *text_data, GRect bounds) {
  GRect text_bounds = bounds;
  
  // Draw white outline (4 directions: up, down, left, right)
  graphics_context_set_text_color(ctx, GColorWhite);
  
  // Shift left
  text_bounds.origin.x = bounds.origin.x - 1;
  text_bounds.origin.y = bounds.origin.y;
  graphics_draw_text(ctx, text_data->text, text_data->font, text_bounds, 
                     GTextOverflowModeTrailingEllipsis, text_data->alignment, NULL);
  
  // Shift right
  text_bounds.origin.x = bounds.origin.x + 1;
  text_bounds.origin.y = bounds.origin.y;
  graphics_draw_text(ctx, text_data->text, text_data->font, text_bounds, 
                     GTextOverflowModeTrailingEllipsis, text_data->alignment, NULL);
  
  // Shift up
  text_bounds.origin.x = bounds.origin.x;
  text_bounds.origin.y = bounds.origin.y - 1;
  graphics_draw_text(ctx, text_data->text, text_data->font, text_bounds, 
                     GTextOverflowModeTrailingEllipsis, text_data->alignment, NULL);
  
  // Shift down
  text_bounds.origin.x = bounds.origin.x;
  text_bounds.origin.y = bounds.origin.y + 1;
  graphics_draw_text(ctx, text_data->text, text_data->font, text_bounds, 
                     GTextOverflowModeTrailingEllipsis, text_data->alignment, NULL);
  
  // Draw black text in center
  graphics_context_set_text_color(ctx, GColorBlack);
  text_bounds.origin.x = bounds.origin.x;
  text_bounds.origin.y = bounds.origin.y;
  graphics_draw_text(ctx, text_data->text, text_data->font, text_bounds, 
                     GTextOverflowModeTrailingEllipsis, text_data->alignment, NULL);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  strftime(s_time_text.text, sizeof(s_time_text.text), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  // Write the current date (2 lines: MM/dd, then year)
  strftime(s_date_line1, sizeof(s_date_line1), "%m/%d", tick_time);
  strftime(s_date_line2, sizeof(s_date_line2), "%Y", tick_time);
  
  // Trigger redraw
  layer_mark_dirty(s_time_layer);
  layer_mark_dirty(s_date_layer);
}

static void update_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  // Check if health data is available
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int steps = (int)health_service_sum_today(metric);
    int steps_k = steps / 1000;
    snprintf(s_steps_line1, sizeof(s_steps_line1), "%dk", steps_k);
  } else {
    snprintf(s_steps_line1, sizeof(s_steps_line1), "--k");
  }
  
  snprintf(s_steps_line2, sizeof(s_steps_line2), "STEP");
  
  // Trigger redraw
  if (s_steps_value_layer) {
    layer_mark_dirty(s_steps_value_layer);
  }
  if (s_steps_label_layer) {
    layer_mark_dirty(s_steps_label_layer);
  }
}

static void update_battery() {
  BatteryChargeState charge_state = battery_state_service_peek();
  
  snprintf(s_battery_line1, sizeof(s_battery_line1), "%d", charge_state.charge_percent);
  snprintf(s_battery_line2, sizeof(s_battery_line2), "BAT");
  
  // Trigger redraw
  if (s_battery_value_layer) {
    layer_mark_dirty(s_battery_value_layer);
  }
  if (s_battery_label_layer) {
    layer_mark_dirty(s_battery_label_layer);
  }
}

static void battery_handler(BatteryChargeState charge_state) {
  update_battery();
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    update_steps();
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  update_steps();
  update_battery();
}

// Layer update callbacks
static void time_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  draw_outlined_text(ctx, &s_time_text, bounds);
}

static void date_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int line_height = 18;
  
  // Draw first line (day of week)
  GRect line1_bounds = GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, line_height);
  
  // White outline for line 1
  graphics_context_set_text_color(ctx, GColorWhite);
  GRect text_bounds = GRect(line1_bounds.origin.x - 1, line1_bounds.origin.y, line1_bounds.size.w, line1_bounds.size.h);
  graphics_draw_text(ctx, s_date_line1, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  text_bounds = GRect(line1_bounds.origin.x + 1, line1_bounds.origin.y, line1_bounds.size.w, line1_bounds.size.h);
  graphics_draw_text(ctx, s_date_line1, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  text_bounds = GRect(line1_bounds.origin.x, line1_bounds.origin.y - 1, line1_bounds.size.w, line1_bounds.size.h);
  graphics_draw_text(ctx, s_date_line1, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  text_bounds = GRect(line1_bounds.origin.x, line1_bounds.origin.y + 1, line1_bounds.size.w, line1_bounds.size.h);
  graphics_draw_text(ctx, s_date_line1, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  
  // Black text for line 1
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_date_line1, s_date_text.font, line1_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  
  // Draw second line (date and month)
  GRect line2_bounds = GRect(bounds.origin.x, bounds.origin.y + line_height, bounds.size.w, line_height);
  
  // White outline for line 2
  graphics_context_set_text_color(ctx, GColorWhite);
  text_bounds = GRect(line2_bounds.origin.x - 1, line2_bounds.origin.y, line2_bounds.size.w, line2_bounds.size.h);
  graphics_draw_text(ctx, s_date_line2, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  text_bounds = GRect(line2_bounds.origin.x + 1, line2_bounds.origin.y, line2_bounds.size.w, line2_bounds.size.h);
  graphics_draw_text(ctx, s_date_line2, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  text_bounds = GRect(line2_bounds.origin.x, line2_bounds.origin.y - 1, line2_bounds.size.w, line2_bounds.size.h);
  graphics_draw_text(ctx, s_date_line2, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  text_bounds = GRect(line2_bounds.origin.x, line2_bounds.origin.y + 1, line2_bounds.size.w, line2_bounds.size.h);
  graphics_draw_text(ctx, s_date_line2, s_date_text.font, text_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
  
  // Black text for line 2
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_date_line2, s_date_text.font, line2_bounds, GTextOverflowModeTrailingEllipsis, s_date_text.alignment, NULL);
}

static void steps_value_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GFont small_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  // Black text
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_steps_line1, small_font, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void steps_label_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GFont small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  // Black text
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_steps_line2, small_font, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void battery_value_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GFont small_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  // Black text
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_battery_line1, small_font, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void battery_label_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GFont small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  // Black text
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_battery_line2, small_font, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

// Metal effect layer drawing function
static void metal_effect_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Create a brushed metal effect with alternating lighter/darker horizontal lines
  // Using a semi-transparent overlay to give a metallic sheen
  for (int y = bounds.origin.y; y < bounds.size.h; y++) {
    // Create a pattern that repeats every 4 pixels for brushed metal look
    int pattern = y % 4;
    GColor color;
    
    switch(pattern) {
      case 0:
        color = GColorLightGray;
        break;
      case 1:
        color = GColorWhite;
        break;
      case 2:
        color = GColorLightGray;
        break;
      case 3:
        color = GColorDarkGray;
        break;
      default:
        color = GColorLightGray;
    }
    
    graphics_context_set_stroke_color(ctx, color);
    graphics_draw_line(ctx, GPoint(bounds.origin.x, y), GPoint(bounds.size.w, y));
  }
  
  // Add some vertical highlights for additional metallic effect
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (int x = 20; x < bounds.size.w; x += 40) {
    graphics_draw_line(ctx, GPoint(x, 0), GPoint(x, bounds.size.h));
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set background color to light gray
  window_set_background_color(s_window, GColorLightGray);

  // Create metal effect layer above background
  s_metal_effect_layer = layer_create(bounds);
  layer_set_update_proc(s_metal_effect_layer, metal_effect_layer_update);
  layer_add_child(window_layer, s_metal_effect_layer);

  // Load and display background image
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  // Initialize time text
  snprintf(s_time_text.text, sizeof(s_time_text.text), "00:00");
  s_time_text.font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  s_time_text.alignment = GTextAlignmentCenter;
  s_time_layer = layer_create(GRect(0, 46, bounds.size.w, 50));
  layer_set_update_proc(s_time_layer, time_layer_update);
  layer_add_child(window_layer, s_time_layer);

  // Initialize date text
  snprintf(s_date_text.text, sizeof(s_date_text.text), "Loading...");
  s_date_text.font = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
  s_date_text.alignment = GTextAlignmentCenter;
  s_date_layer = layer_create(GRect(68, 2, 80, 65));
  layer_set_update_proc(s_date_layer, date_layer_update);
  layer_add_child(window_layer, s_date_layer);

  // Initialize steps display - value layer
  snprintf(s_steps_line1, sizeof(s_steps_line1), "--k");
  s_steps_value_layer = layer_create(GRect(8, 105, 60, 20));
  layer_set_update_proc(s_steps_value_layer, steps_value_layer_update);
  layer_add_child(window_layer, s_steps_value_layer);

  // Initialize steps display - label layer
  snprintf(s_steps_line2, sizeof(s_steps_line2), "STEP");
  s_steps_label_layer = layer_create(GRect(8, 128, 60, 20));
  layer_set_update_proc(s_steps_label_layer, steps_label_layer_update);
  layer_add_child(window_layer, s_steps_label_layer);

  // Initialize battery display - value layer
  snprintf(s_battery_line1, sizeof(s_battery_line1), "--");
  s_battery_value_layer = layer_create(GRect(79, 105, 60, 20));
  layer_set_update_proc(s_battery_value_layer, battery_value_layer_update);
  layer_add_child(window_layer, s_battery_value_layer);

  // Initialize battery display - label layer
  snprintf(s_battery_line2, sizeof(s_battery_line2), "BAT");
  s_battery_label_layer = layer_create(GRect(79, 128, 60, 20));
  layer_set_update_proc(s_battery_label_layer, battery_label_layer_update);
  layer_add_child(window_layer, s_battery_label_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_time_layer);
  layer_destroy(s_date_layer);
  layer_destroy(s_vault_tec_layer);
  layer_destroy(s_tagline_layer);
  layer_destroy(s_steps_value_layer);
  layer_destroy(s_steps_label_layer);
  layer_destroy(s_battery_value_layer);
  layer_destroy(s_battery_label_layer);
  layer_destroy(s_metal_effect_layer);
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Subscribe to health events
  health_service_events_subscribe(health_handler, NULL);

  // Subscribe to battery events
  battery_state_service_subscribe(battery_handler);

  // Make sure the time is displayed from the start
  update_time();
  update_steps();
  update_battery();
}

static void prv_deinit(void) {
  health_service_events_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
