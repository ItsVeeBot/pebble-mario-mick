// Copyright (C) 2013 Denis Dzyubenko
// Copyright (C) 2015 Alexey Avdyukhin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// You may contact the author of the original watchface at denis@ddenis.info
// Author of the color mod for Pebble Time: clusterrr@clusterrr.com
//

#include <pebble.h>
#include <time.h>

//#define DEMO // display fake time. Good for taking screenshots of the watchface.
//#define DEMO_SLOW // slow movements

static Window *window;

static Layer *blocks_layer;
static Layer *mario_layer;
static Layer *time_layer;
static Layer *background_layer;
static Layer *battery_layer;
static Layer *phone_battery_layer;

static char hour_text[3];
static char minute_text[3];
static char hour_text_visible[]   = "  ";
static char minute_text_visible[] = "  ";
static char date_text[12];

static GRect blocks_down_rect;
static GRect blocks_up_rect;
static GRect mario_down_rect;
static GRect mario_up_rect;
static GRect background_rect;
static GRect battery_rect;
static GRect phone_battery_rect;
static GRect time_up_rect;
static GRect time_normal_rect;
static GRect time_down_rect;

static GFont pixel_font;
static GFont pixel_font_small;

static int mario_is_down = 1;
static int need_update_background = 0;

// TODO: I can really make use of BitmapLayer here
static GBitmap *mario_normal_bmp = NULL;
static GBitmap *mario_jump_bmp = NULL;
static GBitmap *block_bmp = NULL;
static GBitmap *no_phone_bmp = NULL;
static GBitmap *phone_bmp = NULL;
static GBitmap *watch_bmp = NULL;
static GBitmap *battery_charging_bmp = NULL;
static GBitmap *background_day_bmp = NULL;

static PropertyAnimation *mario_animation_beg = NULL;
static PropertyAnimation *mario_animation_end = NULL;

static PropertyAnimation *block_animation_beg = NULL;
static PropertyAnimation *block_animation_end = NULL;

static PropertyAnimation *hour_animation_slide_away = NULL;
static PropertyAnimation *hour_animation_slide_in = NULL;

static bool config_show_no_phone = true;
static bool config_show_battery = true;
static bool config_show_phone_battery = true;
static bool config_vibe = false;
static int config_background = 0;
static int phone_battery_level = -1;

#define BLOCK_SIZE 50
#define BLOCK_SPACING 0
#ifdef DEMO_SLOW
#define MARIO_JUMP_DURATION 5000
#define CLOCK_ANIMATION_DURATION 5000
#else
#define MARIO_JUMP_DURATION 150
#define CLOCK_ANIMATION_DURATION 150
#endif
#define GROUND_HEIGHT 26

#define MSG_SHOW_NO_PHONE 0
#define MSG_SHOW_BATTERY 1
#define MSG_VIBE 2
#define MSG_BATTERY_REQUEST 4
#define MSG_BATTERY_ANSWER 5
#define MSG_SHOW_PHONE_BATTERY 6
#define MSG_BACKGROUND 7

void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void to_upcase(char* str)
{
  while (*str)
  {
    if (*str >='a' && *str <= 'z') *str += 'A'-'a';
    str++;
  }
}

static void request_phone_battery()
{
  if (config_show_phone_battery)
  {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  Tuplet tupleRequest = TupletInteger(MSG_BATTERY_REQUEST, 0);
  dict_write_tuplet(iter, &tupleRequest);
  app_message_outbox_send();
  }
}

void time_update_callback(Layer *layer, GContext *ctx)
{
  GRect layer_bounds = layer_get_bounds(layer);
  char h1[2];
  char h2[2];
  char m1[2];
  char m2[2];
  h1[0] = hour_text_visible[0];
  h2[0] = hour_text_visible[1];
  m1[0] = minute_text_visible[0];
  m2[0] = minute_text_visible[1];
  h1[1] = h2[1] = m1[1] = m2[1] = 0;
  layer_bounds.origin.y += 1;
  int one_fix = 2;
#if PBL_COLOR  
  GRect origin = layer_bounds;
  graphics_context_set_text_color(ctx, GColorBlack);
  layer_bounds.origin.x += 4;
  layer_bounds.origin.y += 1;
  if (h1[0] == '1') layer_bounds.origin.x -= one_fix;
  graphics_draw_text(ctx, h1, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (h1[0] == '1') layer_bounds.origin.x += one_fix;
  layer_bounds.origin.x += 20;
  if (h2[0] == '1') layer_bounds.origin.x += one_fix;
  graphics_draw_text(ctx, h2, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (h2[0] == '1') layer_bounds.origin.x -= one_fix;
  layer_bounds.origin.x += 30;
  if (m1[0] == '1') layer_bounds.origin.x -= one_fix;
  graphics_draw_text(ctx, m1, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (m1[0] == '1') layer_bounds.origin.x += one_fix;
  layer_bounds.origin.x += 20;
  if (m2[0] == '1') layer_bounds.origin.x += one_fix;
  graphics_draw_text(ctx, m2, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  layer_bounds = origin;
  graphics_context_set_text_color(ctx, GColorWindsorTan);
  layer_bounds.origin.x += 3;
  if (h1[0] == '1') layer_bounds.origin.x -= one_fix;
  graphics_draw_text(ctx, h1, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (h1[0] == '1') layer_bounds.origin.x += one_fix;
  layer_bounds.origin.x += 20;
  if (h2[0] == '1') layer_bounds.origin.x += one_fix;
  graphics_draw_text(ctx, h2, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (h2[0] == '1') layer_bounds.origin.x -= one_fix;
  layer_bounds.origin.x += 30;
  if (m1[0] == '1') layer_bounds.origin.x -= one_fix;
  graphics_draw_text(ctx, m1, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (m1[0] == '1') layer_bounds.origin.x += one_fix;
  layer_bounds.origin.x += 20;
  if (m2[0] == '1') layer_bounds.origin.x += one_fix;
  graphics_draw_text(ctx, m2, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
#else
  graphics_context_set_text_color(ctx, GColorBlack);
  layer_bounds.origin.x += 3;
  if (h1[0] == '1') layer_bounds.origin.x -= one_fix;
  graphics_draw_text(ctx, h1, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (h1[0] == '1') layer_bounds.origin.x += one_fix;
  layer_bounds.origin.x += 20;
  if (h2[0] == '1') layer_bounds.origin.x += one_fix;
  graphics_draw_text(ctx, h2, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (h2[0] == '1') layer_bounds.origin.x -= one_fix;
  layer_bounds.origin.x += 30;
  if (m1[0] == '1') layer_bounds.origin.x -= one_fix;
  graphics_draw_text(ctx, m1, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (m1[0] == '1') layer_bounds.origin.x += one_fix;
  layer_bounds.origin.x += 20;
  if (m2[0] == '1') layer_bounds.origin.x += 3;
  graphics_draw_text(ctx, m2, pixel_font, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
#endif
}

void blocks_update_callback(Layer *layer, GContext *ctx)
{
  GRect block_rect[2];

  GRect layer_bounds = layer_get_bounds(layer);
  GRect layer_frame = layer_get_frame(layer);

  block_rect[0] = GRect(layer_bounds.origin.x,
              layer_bounds.origin.y + 4,
              BLOCK_SIZE,
              layer_frame.size.h - 4);
  block_rect[1] = GRect(layer_bounds.origin.x + BLOCK_SIZE + BLOCK_SPACING,
              layer_bounds.origin.y + 4,
              BLOCK_SIZE,
              layer_frame.size.h - 4);

#if PBL_COLOR
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
  for (uint8_t i = 0; i < 2; ++i) {
    GRect *rect = block_rect + i;
    graphics_draw_bitmap_in_rect(ctx, block_bmp, *rect);
  }
}

void mario_update_callback(Layer *layer, GContext *ctx)
{
  GRect destination;
  GBitmap *bmp;

  bmp = mario_is_down ? mario_normal_bmp : mario_jump_bmp;
  destination = GRect(0, 0, gbitmap_get_bounds(bmp).size.w, gbitmap_get_bounds(bmp).size.h);

#if PBL_COLOR
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
  graphics_context_set_compositing_mode(ctx, GCompOpAnd);
#endif
  graphics_draw_bitmap_in_rect(ctx, bmp, destination);
}

// Draw background and date
void ground_update_callback(Layer *layer, GContext *ctx)
{
  GRect layer_bounds = layer_get_bounds(layer);

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, background_day_bmp, layer_bounds);

  layer_bounds.origin.y = 5;
  layer_bounds.origin.x = 31;
#if PBL_COLOR  
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, GColorBlack);
#endif

  time_t t;
  time(&t); 
  struct tm * tick_time = localtime(&t);
  
  // Compress spaces
  strftime(date_text, sizeof(date_text), "%a,", tick_time);
  to_upcase(date_text);
  graphics_draw_text(ctx, date_text, pixel_font_small, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  layer_bounds.origin.x += 34;
  strftime(date_text, sizeof(date_text), "%b", tick_time);
  to_upcase(date_text);
  graphics_draw_text(ctx, date_text, pixel_font_small, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  layer_bounds.origin.x += 30;
  strftime(date_text, sizeof(date_text), "%d", tick_time);
  to_upcase(date_text);
  graphics_draw_text(ctx, date_text, pixel_font_small, layer_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

void bluetooth_connection_callback(bool connected)
{
  if (config_vibe && !connected) {  
    static const uint32_t const segments[] = { 100, 200, 100, 200, 100 };
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
    };    
    vibes_enqueue_custom_pattern(pat);
  }
  
  if (connected)
    request_phone_battery();
  else phone_battery_level = -1;
  layer_mark_dirty(phone_battery_layer);
}

void phone_battery_update_callback(Layer *layer, GContext *ctx)
{
#ifdef DEMO
  phone_battery_level = 8;
#endif
#if PBL_COLOR
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
  if (config_show_no_phone && !bluetooth_connection_service_peek())
  {
    GRect image_rect = gbitmap_get_bounds(no_phone_bmp);
    graphics_draw_bitmap_in_rect(ctx, no_phone_bmp, image_rect);
  }  
  else if (phone_battery_level >= 0 && config_show_phone_battery)
  {
    GRect image_rect = gbitmap_get_bounds(phone_bmp);
    graphics_draw_bitmap_in_rect(ctx, phone_bmp, image_rect);
#if PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorWhite);
#else
    graphics_context_set_fill_color(ctx, GColorBlack);
#endif
    graphics_fill_rect(ctx, GRect(9, 2, phone_battery_level, 5), 0, GCornerNone);
  }
}

void battery_update_callback(Layer *layer, GContext *ctx)
{
  if (config_show_battery)
  {
    GRect image_rect = gbitmap_get_bounds(watch_bmp);
    BatteryChargeState charge_state = battery_state_service_peek();
#if PBL_COLOR
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
    if (!charge_state.is_charging)
      graphics_draw_bitmap_in_rect(ctx, watch_bmp, image_rect);
    else
      graphics_draw_bitmap_in_rect(ctx, battery_charging_bmp, image_rect);
#if PBL_COLOR
      graphics_context_set_fill_color(ctx, GColorWhite);
#else
      graphics_context_set_fill_color(ctx, GColorBlack);
#endif
    graphics_fill_rect(ctx, GRect(9, 2, charge_state.charge_percent / 10, 5), 0, GCornerNone);
  }
}

void handle_battery(BatteryChargeState charge_state)
{
  layer_mark_dirty(battery_layer);
}

void update_background()
{
  if (background_day_bmp)
  gbitmap_destroy(background_day_bmp);
#if PBL_COLOR
  if (!config_background)
  {
  time_t t;
  time(&t); 
  struct tm * tick_time = localtime(&t);
#ifdef DEMO
  int s = (tick_time->tm_sec + tick_time->tm_min*60) / 15;
  int hour = ((s % 12) / 2 * 4 + 1);
#else
  int hour = tick_time->tm_hour;
#endif
  if (hour < 4)
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_UNDERGROUND);
  else if (hour < 8)
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_CASTLE);
  else if (hour < 20)
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_DAY);
  else
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_NIGHT);
  } else {
  switch (config_background)
  {
    default: 
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_DAY);
    break;
    case 2: 
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_NIGHT);
    break;
    case 3: 
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_UNDERGROUND);
    break;
    case 4: 
    background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_CASTLE);
    break;
  }
  }
#else
  background_day_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_DAY);   
#endif
}

void load_bitmaps()
{
  if (mario_normal_bmp)
    gbitmap_destroy(mario_normal_bmp);
  if (mario_jump_bmp)
    gbitmap_destroy(mario_jump_bmp);
  if (no_phone_bmp)
    gbitmap_destroy(no_phone_bmp);
  if (phone_bmp)
    gbitmap_destroy(phone_bmp);
  if (watch_bmp)
    gbitmap_destroy(watch_bmp);
  if (battery_charging_bmp)
    gbitmap_destroy(battery_charging_bmp);
  if (block_bmp)
    gbitmap_destroy(block_bmp);

  mario_normal_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_NORMAL);
  mario_jump_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_JUMP);
  no_phone_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_PHONE);
  phone_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PHONE_ICON);
  watch_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WATCH_ICON);
  battery_charging_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGING);
  block_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLOCK);
  update_background();
}

void in_received_handler(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received config");
  Tuple *tuple = dict_find(received, MSG_SHOW_NO_PHONE);
  if (tuple) {
    if (tuple->type == TUPLE_CSTRING)
      config_show_no_phone = (strcmp(tuple->value->cstring, "true") == 0);
    else
      config_show_no_phone = tuple->value->int8;
    layer_mark_dirty(phone_battery_layer);
    persist_write_bool(MSG_SHOW_NO_PHONE, config_show_no_phone);
  }
  tuple = dict_find(received, MSG_SHOW_BATTERY);
  if (tuple) {
    if (tuple->type == TUPLE_CSTRING)
      config_show_battery = (strcmp(tuple->value->cstring, "true") == 0);
    else
      config_show_battery = tuple->value->int8;
    layer_mark_dirty(battery_layer);
    persist_write_bool(MSG_SHOW_BATTERY, config_show_battery);
  }
  tuple = dict_find(received, MSG_SHOW_PHONE_BATTERY);
  if (tuple) {
    if (tuple->type == TUPLE_CSTRING)
      config_show_phone_battery = (strcmp(tuple->value->cstring, "true") == 0);
    else
      config_show_phone_battery = tuple->value->int8;
    if (config_show_phone_battery) request_phone_battery();
    layer_mark_dirty(phone_battery_layer);
    persist_write_bool(MSG_SHOW_PHONE_BATTERY, config_show_phone_battery);
  }
  tuple = dict_find(received, MSG_VIBE);
  if (tuple) {
    if (tuple->type == TUPLE_CSTRING)
      config_vibe = (strcmp(tuple->value->cstring, "true") == 0);
    else
      config_vibe = tuple->value->int8;
    persist_write_bool(MSG_VIBE, config_vibe);
  }
  tuple = dict_find(received, MSG_BACKGROUND);
  if (tuple) {
    config_background = tuple->value->int8;
    update_background();
    layer_mark_dirty(background_layer);
    persist_write_bool(MSG_BACKGROUND, config_background);
  }
  tuple = dict_find(received, MSG_BATTERY_ANSWER);
  if (tuple) {
    phone_battery_level = tuple->value->int8;
    layer_mark_dirty(phone_battery_layer);
  }
}

void handle_init()
{
  if (persist_exists(MSG_SHOW_NO_PHONE))
    config_show_no_phone = persist_read_bool(MSG_SHOW_NO_PHONE);
  if (persist_exists(MSG_SHOW_BATTERY))
    config_show_battery = persist_read_bool(MSG_SHOW_BATTERY);
  if (persist_exists(MSG_SHOW_PHONE_BATTERY))
    config_show_phone_battery = persist_read_bool(MSG_SHOW_PHONE_BATTERY);
  if (persist_exists(MSG_VIBE))
    config_vibe = persist_read_bool(MSG_VIBE);
  if (persist_exists(MSG_BACKGROUND))
    config_background = persist_read_bool(MSG_BACKGROUND);  

  app_message_register_inbox_received(in_received_handler);
  app_message_open(64, 64);

  window = window_create();

  blocks_up_rect = GRect(22, -10, BLOCK_SIZE*2, BLOCK_SIZE + 4);
#if PBL_COLOR
  mario_down_rect = GRect(32 + 15, 168-GROUND_HEIGHT-76 + 28 + 10, 80, 80);
  mario_up_rect = GRect(32 + 15, BLOCK_SIZE + 4 + 10, 80, 80);
  blocks_down_rect = GRect(22, 25, BLOCK_SIZE*2, BLOCK_SIZE + 4);
#else
  mario_down_rect = GRect(32, 168-GROUND_HEIGHT-80 + 10, 80, 80);
  mario_up_rect = GRect(32, BLOCK_SIZE + 4, 80, 80);
  blocks_down_rect = GRect(22, 16, BLOCK_SIZE*2, BLOCK_SIZE + 4);
#endif
  
  background_rect = GRect(0, 0, 144, 168);
  battery_rect = GRect(119, 5, 22, 9);
  phone_battery_rect = GRect(3, 5, 22, 9);

  pixel_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GAMEGIRL_24));
  //pixel_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EMULOGIC_24));
  pixel_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EMULOGIC_8));
  
  time_up_rect = GRect(0, -15, BLOCK_SIZE*2, BLOCK_SIZE);
  time_normal_rect = GRect(0, 5 + 4 + 5, BLOCK_SIZE*2, BLOCK_SIZE);
  time_down_rect = GRect(0, BLOCK_SIZE + 4 + 5, BLOCK_SIZE*2, BLOCK_SIZE);

  blocks_layer = layer_create(blocks_down_rect);
  mario_layer = layer_create(mario_down_rect);
  background_layer = layer_create(background_rect);
  battery_layer = layer_create(battery_rect);
  phone_battery_layer = layer_create(phone_battery_rect);
  time_layer = layer_create(time_normal_rect);

  layer_set_update_proc(blocks_layer, &blocks_update_callback);
  layer_set_update_proc(mario_layer, &mario_update_callback);
  layer_set_update_proc(background_layer, &ground_update_callback);
  layer_set_update_proc(battery_layer, &battery_update_callback);
  layer_set_update_proc(phone_battery_layer, &phone_battery_update_callback);
  layer_set_update_proc(time_layer, &time_update_callback);

  Layer *window_layer = window_get_root_layer(window);

  layer_add_child(window_layer, background_layer);
  layer_add_child(background_layer, battery_layer);
  layer_add_child(background_layer, phone_battery_layer);
  layer_add_child(blocks_layer, time_layer);
  layer_add_child(background_layer, blocks_layer);
  layer_add_child(background_layer, mario_layer);
  
  window_stack_push(window, false);

#ifdef DEMO
  #define MARIO_TIME_UNIT SECOND_UNIT
#else
  #define MARIO_TIME_UNIT MINUTE_UNIT
#endif

  phone_battery_level = -1;

  tick_timer_service_subscribe(MARIO_TIME_UNIT, handle_tick);  
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  battery_state_service_subscribe(handle_battery);
  
  request_phone_battery();

   load_bitmaps(); 
#ifdef DEMO
  light_enable(true);
#endif
}

void handle_deinit()
{
  tick_timer_service_unsubscribe();

  animation_unschedule_all();

  if (mario_animation_beg)
    property_animation_destroy(mario_animation_beg);
  if (mario_animation_end)
    property_animation_destroy(mario_animation_end);  
  if (block_animation_beg)
    property_animation_destroy(block_animation_beg);
  if (block_animation_end)
    property_animation_destroy(block_animation_end);
  if (hour_animation_slide_in)
    property_animation_destroy(hour_animation_slide_in);
  if (hour_animation_slide_away)
    property_animation_destroy(hour_animation_slide_away);

  gbitmap_destroy(mario_normal_bmp);
  gbitmap_destroy(mario_jump_bmp);
  gbitmap_destroy(no_phone_bmp);
  gbitmap_destroy(phone_bmp);
  gbitmap_destroy(watch_bmp);
  gbitmap_destroy(battery_charging_bmp);  
  gbitmap_destroy(block_bmp);
  gbitmap_destroy(background_day_bmp);

  layer_destroy(time_layer);
  layer_destroy(background_layer);
  layer_destroy(mario_layer);
  layer_destroy(blocks_layer);
  layer_destroy(battery_layer);
  layer_destroy(phone_battery_layer);

  fonts_unload_custom_font(pixel_font);
  fonts_unload_custom_font(pixel_font_small);
  
  window_destroy(window);
  
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  app_message_deregister_callbacks();
}

void mario_down_animation_stopped(Animation *animation, void *data)
{
  mario_is_down = 1;
  layer_mark_dirty(mario_layer);
}

void mario_jump_animation_started(Animation *animation, void *data)
{
  mario_is_down = 0;
  layer_mark_dirty(mario_layer);
}

void mario_jump_animation_stopped(Animation *animation, void *data)
{
  if (mario_animation_end)
    property_animation_destroy(mario_animation_end);

  mario_animation_end = property_animation_create_layer_frame(mario_layer,
                                  &mario_up_rect,
                                  &mario_down_rect);
  animation_set_duration((Animation *)mario_animation_end, MARIO_JUMP_DURATION);
  animation_set_curve((Animation *)mario_animation_end, AnimationCurveEaseIn);
  animation_set_handlers((Animation *)mario_animation_end, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)mario_down_animation_stopped
  }, 0);

  animation_schedule((Animation *)mario_animation_end);
}

void clock_animation_slide_away_stopped(Animation *animation, void *data)
{
  layer_set_frame((Layer *)time_layer, time_down_rect);
  strcpy(hour_text_visible, hour_text);
  strcpy(minute_text_visible, minute_text);
  layer_mark_dirty(time_layer);

  if (hour_animation_slide_in)
    property_animation_destroy(hour_animation_slide_in);

  hour_animation_slide_in = property_animation_create_layer_frame((Layer *)time_layer,
                                    &time_down_rect,
                                    &time_normal_rect);
  animation_set_duration((Animation *)hour_animation_slide_in, CLOCK_ANIMATION_DURATION);
  animation_set_curve((Animation *)hour_animation_slide_in, AnimationCurveLinear);
  animation_schedule((Animation *)hour_animation_slide_in);
}

void block_up_animation_started(Animation *animation, void *data)
{
  if (hour_animation_slide_away)
    property_animation_destroy(hour_animation_slide_away);
  hour_animation_slide_away = property_animation_create_layer_frame((Layer *)time_layer,
                                      &time_normal_rect,
                                      &time_up_rect);
  animation_set_duration((Animation *)hour_animation_slide_away, CLOCK_ANIMATION_DURATION);
  animation_set_curve((Animation *)hour_animation_slide_away, AnimationCurveLinear);
  animation_set_handlers((Animation *)hour_animation_slide_away, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)clock_animation_slide_away_stopped
  }, 0);
  animation_schedule((Animation *)hour_animation_slide_away);
}

void block_up_animation_stopped(Animation *animation, void *data)
{
  if (block_animation_end)
    property_animation_destroy(block_animation_end);
  block_animation_end = property_animation_create_layer_frame(blocks_layer,
                                  &blocks_up_rect,
                                  &blocks_down_rect);
  animation_set_duration((Animation *)block_animation_end, MARIO_JUMP_DURATION);
  animation_set_curve((Animation *)block_animation_end, AnimationCurveEaseIn);
  animation_schedule((Animation *)block_animation_end);

  // Update background if need    
  if (need_update_background)
  {
    need_update_background = 0;
    update_background();
  }
  layer_mark_dirty(background_layer);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
#ifdef DEMO
  if (tick_time->tm_sec % 15 != 0) return;
#endif
  if (mario_animation_beg)
    property_animation_destroy(mario_animation_beg);
  mario_animation_beg = property_animation_create_layer_frame(mario_layer,
                                  &mario_down_rect,
                                  &mario_up_rect);
  animation_set_duration((Animation *)mario_animation_beg, MARIO_JUMP_DURATION);
  animation_set_curve((Animation *)mario_animation_beg, AnimationCurveEaseOut);
  animation_set_handlers((Animation *)mario_animation_beg, (AnimationHandlers){
    .started = (AnimationStartedHandler)mario_jump_animation_started,
    .stopped = (AnimationStoppedHandler)mario_jump_animation_stopped
  }, 0);

  if (block_animation_beg)
    property_animation_destroy(block_animation_beg);

  block_animation_beg = property_animation_create_layer_frame(blocks_layer,
                                  &blocks_down_rect,
                                  &blocks_up_rect);
  animation_set_duration((Animation *)block_animation_beg, MARIO_JUMP_DURATION);
#if PBL_COLOR
  animation_set_delay((Animation *)block_animation_beg, MARIO_JUMP_DURATION*4/9);
#else
  animation_set_delay((Animation *)block_animation_beg, MARIO_JUMP_DURATION/10);
#endif
  animation_set_curve((Animation *)block_animation_beg, AnimationCurveEaseOut);
  animation_set_handlers((Animation *)block_animation_beg, (AnimationHandlers){
    .started = (AnimationStartedHandler)block_up_animation_started,
    .stopped = (AnimationStoppedHandler)block_up_animation_stopped
  }, 0);
  
#ifdef DEMO
  int s = (tick_time->tm_sec + tick_time->tm_min*60) / 15;
  snprintf(hour_text, sizeof(hour_text), "%02d", ((s % 12) / 2 * 4 + 1));
  snprintf(minute_text, sizeof(minute_text), "%02d", s % 60);
#else
  char *hour_format;
  if (clock_is_24h_style()) {
    hour_format = "%H";
  } else {
    hour_format = "%I";
  }
  strftime(hour_text, sizeof(hour_text), hour_format, tick_time);
  char *minute_format = "%M";
  strftime(minute_text, sizeof(minute_text), minute_format, tick_time);
#endif

  animation_schedule((Animation *)mario_animation_beg);
  animation_schedule((Animation *)block_animation_beg);
  
  if ((units_changed | MINUTE_UNIT) && (tick_time->tm_min % 30 == 0))
    request_phone_battery();
  
  if (units_changed | HOUR_UNIT)
    need_update_background = 1;
}

int main(void)
{
  handle_init();
  app_event_loop();
  handle_deinit();
}
