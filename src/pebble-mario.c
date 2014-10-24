// Copyright (C) 2013 Denis Dzyubenko
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
// You may contact the author at denis@ddenis.info
//

#include <pebble.h>
#include <time.h>

// #define DEMO // display fake time. Good for taking screenshots of the watchface.

Window *window;

Layer *blocks_layer;
Layer *mario_layer;
TextLayer *text_hour_layer;
TextLayer *text_minute_layer;
Layer *ground_layer;
TextLayer *date_layer;
Layer *no_phone_layer;
Layer *battery_layer;

static char hour_text[]   = "00";
static char minute_text[] = "00";
static char date_text[] = "Wed, Apr 17";

GRect blocks_down_rect;
GRect blocks_up_rect;
GRect mario_down_rect;
GRect mario_up_rect;
GRect ground_rect;
GRect no_phone_rect;
GRect battery_rect;

GRect hour_up_rect;
GRect hour_normal_rect;
GRect hour_down_rect;
GRect minute_up_rect;
GRect minute_normal_rect;
GRect minute_down_rect;

static int mario_is_down = 1;

// TODO: I can really make use of BitmapLayer here
GBitmap *mario_normal_bmp = NULL;
GBitmap *mario_jump_bmp = NULL;
GBitmap *ground_bmp = NULL;
GBitmap *no_phone_bmp = NULL;
GBitmap *battery_bmp = NULL;
GBitmap *battery_charging_bmp = NULL;

PropertyAnimation *mario_animation_beg;
PropertyAnimation *mario_animation_end;

PropertyAnimation *block_animation_beg;
PropertyAnimation *block_animation_end;

PropertyAnimation *hour_animation_slide_away;
PropertyAnimation *hour_animation_slide_in;
PropertyAnimation *minute_animation_slide_away;
PropertyAnimation *minute_animation_slide_in;

bool config_show_no_phone = true;
bool config_show_battery = true;
bool config_vibe = false;
bool config_inversed = false;

#define BLOCK_SIZE 50
#define BLOCK_LAYER_EXTRA 3
#define BLOCK_SQUEEZE 10
#define BLOCK_SPACING -1
#define MARIO_JUMP_DURATION 50
#define CLOCK_ANIMATION_DURATION 150
#define GROUND_HEIGHT 26

#define MSG_SHOW_NO_PHONE 0
#define MSG_SHOW_BATTERY 1
#define MSG_VIBE 2
#define MSG_INVERSE 3

GColor main_color = GColorBlack;
GColor back_color = GColorWhite;

#if defined(DEMO)
static int demo_advance_time = 0;
#endif

void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void draw_block(GContext *ctx, GRect rect, uint8_t width)
{
    static const uint8_t radius = 1;

    graphics_context_set_fill_color(ctx, main_color);
    graphics_fill_rect(ctx, rect, radius, GCornersAll);

    rect.origin.x += width;
    rect.origin.y += width;
    rect.size.w -= width*2;
    rect.size.h -= width*2;
    graphics_context_set_fill_color(ctx, back_color);
    graphics_fill_rect(ctx, rect, radius, GCornersAll);

    static const uint8_t dot_offset = 3;
    static const uint8_t dot_width = 6;
    static const uint8_t dot_height = 4;

    GRect dot_rect;

    graphics_context_set_fill_color(ctx, main_color);

    // top left dot
    dot_rect = GRect(rect.origin.x + dot_offset, rect.origin.y + dot_offset,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);

    // top right dot
    dot_rect = GRect(rect.origin.x + rect.size.w - dot_offset - dot_width, rect.origin.y + dot_offset,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);

    // bottom left dot
    dot_rect = GRect(rect.origin.x + dot_offset, rect.origin.y + rect.size.h - dot_offset - dot_height,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);

    // bottom right dot
    dot_rect = GRect(rect.origin.x + rect.size.w - dot_offset - dot_width,
                     rect.origin.y + rect.size.h - dot_offset - dot_height,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);
}

void blocks_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect block_rect[2];

    GRect layer_bounds = layer_get_bounds(layer);
    GRect layer_frame = layer_get_frame(layer);

    block_rect[0] = GRect(layer_bounds.origin.x,
                          layer_bounds.origin.y + BLOCK_LAYER_EXTRA,
                          BLOCK_SIZE,
                          layer_frame.size.h - BLOCK_LAYER_EXTRA);
    block_rect[1] = GRect(layer_bounds.origin.x + BLOCK_SIZE + BLOCK_SPACING,
                          layer_bounds.origin.y + BLOCK_LAYER_EXTRA,
                          BLOCK_SIZE,
                          layer_frame.size.h - BLOCK_LAYER_EXTRA);

    for (uint8_t i = 0; i < 2; ++i) {
        GRect *rect = block_rect + i;

        draw_block(ctx, *rect, 4);
    }
}

void mario_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect destination;
    GBitmap *bmp;

    bmp = mario_is_down ? mario_normal_bmp : mario_jump_bmp;
    destination = GRect(0, 0, bmp->bounds.size.w, bmp->bounds.size.h);

    graphics_draw_bitmap_in_rect(ctx, bmp, destination);
}

void ground_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect image_rect = ground_bmp->bounds;
    GRect rect = layer_get_frame(layer);
    int16_t image_width = image_rect.size.w;

    image_rect.origin.x = -10;
    image_rect.origin.y = 0;
    for (int i = 0; i < rect.size.w / image_rect.size.w + 1; ++i) {
        graphics_draw_bitmap_in_rect(ctx, ground_bmp, image_rect);
        image_rect.origin.x +=  image_width;
    }

    text_layer_set_text(date_layer, date_text);
}

void no_phone_update_callback(Layer *layer, GContext *ctx)
{
		if (config_show_no_phone && !bluetooth_connection_service_peek())
		{
				GRect image_rect = no_phone_bmp->bounds;
				graphics_draw_bitmap_in_rect(ctx, no_phone_bmp, image_rect);
		}
}

void bluetooth_connection_callback(bool connected)
{
		layer_mark_dirty(no_phone_layer);
		if (config_vibe && !connected) {		
				static const uint32_t const segments[] = { 100, 200, 100, 200, 100 };
				VibePattern pat = {
						.durations = segments,
						.num_segments = ARRAY_LENGTH(segments),
				};				
				vibes_enqueue_custom_pattern(pat);
		}
}

void battery_update_callback(Layer *layer, GContext *ctx)
{
		if (config_show_battery)
		{
				GRect image_rect = battery_bmp->bounds;
				BatteryChargeState charge_state = battery_state_service_peek();
				if (!charge_state.is_charging)
				{
						graphics_draw_bitmap_in_rect(ctx, battery_bmp, image_rect);
						graphics_context_set_fill_color(ctx, main_color);
						graphics_fill_rect(ctx, GRect(1, 2, charge_state.charge_percent / 10, 6), 0, GCornerNone);
				} else
						graphics_draw_bitmap_in_rect(ctx, battery_charging_bmp, image_rect);
		}
}

void handle_battery(BatteryChargeState charge_state)
{
		layer_mark_dirty(battery_layer);
}

void load_bitmaps()
{
		if (mario_normal_bmp)
				gbitmap_destroy(mario_normal_bmp);
		if (mario_jump_bmp)
				gbitmap_destroy(mario_jump_bmp);
    if (ground_bmp)
				gbitmap_destroy(ground_bmp);
		if (no_phone_bmp)
				gbitmap_destroy(no_phone_bmp);
		if (battery_bmp)
				gbitmap_destroy(battery_bmp);
		if (battery_charging_bmp)
				gbitmap_destroy(battery_charging_bmp);

		if (!config_inversed)
		{
				mario_normal_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_NORMAL);
				mario_jump_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_JUMP);
				ground_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GROUND);
				no_phone_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_PHONE);
				battery_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
				battery_charging_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGING);
		} else {
				mario_normal_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_NORMAL_INVERSED);
				mario_jump_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_JUMP_INVERSED);
				ground_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GROUND_INVERSED);
				no_phone_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_PHONE_INVERSED);
				battery_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_INVERSED);
				battery_charging_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGING_INVERSED);
		}
}

void update_colors()
{
		if (!config_inversed)
		{
				main_color = GColorBlack;
				back_color = GColorWhite;
		} else {
				main_color = GColorWhite;
				back_color = GColorBlack;
		}
		
		window_set_background_color(window, back_color);
		text_layer_set_text_color(text_hour_layer, main_color);
		text_layer_set_text_color(text_minute_layer, main_color);
		text_layer_set_text_color(date_layer, back_color);
		text_layer_set_background_color(date_layer, main_color);
}

void in_received_handler(DictionaryIterator *received, void *context) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received config");
		Tuple *tuple = dict_find(received, MSG_SHOW_NO_PHONE);
		if (tuple) {
				config_show_no_phone = (strcmp(tuple->value->cstring, "true") == 0);
				layer_mark_dirty(no_phone_layer);
		}
		tuple = dict_find(received, MSG_SHOW_BATTERY);
		if (tuple) {
				config_show_battery = (strcmp(tuple->value->cstring, "true") == 0);
				layer_mark_dirty(battery_layer);
		}
		tuple = dict_find(received, MSG_VIBE);
		if (tuple) {
				config_vibe = (strcmp(tuple->value->cstring, "true") == 0);
		}
		tuple = dict_find(received, MSG_INVERSE);
		if (tuple) {
				config_inversed = (strcmp(tuple->value->cstring, "true") == 0);
				load_bitmaps();
				update_colors();
		}
		persist_write_bool(MSG_SHOW_NO_PHONE, config_show_no_phone);
		persist_write_bool(MSG_SHOW_BATTERY, config_show_battery);
		persist_write_bool(MSG_VIBE, config_vibe);
		persist_write_bool(MSG_INVERSE, config_inversed);
}

void handle_init()
{
		if (persist_exists(MSG_SHOW_NO_PHONE))
				config_show_no_phone = persist_read_bool(MSG_SHOW_NO_PHONE);
		if (persist_exists(MSG_SHOW_BATTERY))
				config_show_battery = persist_read_bool(MSG_SHOW_BATTERY);
		if (persist_exists(MSG_VIBE))
				config_vibe = persist_read_bool(MSG_VIBE);
		if (persist_exists(MSG_INVERSE))
				config_inversed = persist_read_bool(MSG_INVERSE);

		app_message_register_inbox_received(in_received_handler);
		app_message_open(64, 64);

    window = window_create();
    window_stack_push(window, true /* Animated */);

    blocks_down_rect = GRect(22, 7, BLOCK_SIZE*2, BLOCK_SIZE + BLOCK_LAYER_EXTRA);
    blocks_up_rect = GRect(22, 0, BLOCK_SIZE*2, BLOCK_SIZE + BLOCK_LAYER_EXTRA - BLOCK_SQUEEZE);
    mario_down_rect = GRect(32, 168-GROUND_HEIGHT-80, 80, 80);
    mario_up_rect = GRect(32, BLOCK_SIZE + BLOCK_LAYER_EXTRA - BLOCK_SQUEEZE, 80, 80);
    ground_rect = GRect(0, 168-GROUND_HEIGHT, 144, 168);
		no_phone_rect = GRect(5, 128, 10, 10);
		battery_rect = GRect(126, 129, 13, 10);

    hour_up_rect = GRect(5, -10, 40, 40);
    hour_normal_rect = GRect(5, 5 + BLOCK_LAYER_EXTRA, 40, 40);
    hour_down_rect = GRect(5, BLOCK_SIZE + BLOCK_LAYER_EXTRA, 40, 40);
    minute_up_rect = GRect(5+BLOCK_SIZE+BLOCK_SPACING, -10, 40, 40);
    minute_normal_rect = GRect(5+BLOCK_SIZE+BLOCK_SPACING, 5 + BLOCK_LAYER_EXTRA, 40, 40);
    minute_down_rect = GRect(5+BLOCK_SIZE+BLOCK_SPACING, BLOCK_SIZE + BLOCK_LAYER_EXTRA, 40, 40);

    blocks_layer = layer_create(blocks_down_rect);
    mario_layer = layer_create(mario_down_rect);
    ground_layer = layer_create(ground_rect);
		no_phone_layer = layer_create(no_phone_rect);
		battery_layer = layer_create(battery_rect);

    layer_set_update_proc(blocks_layer, &blocks_update_callback);
    layer_set_update_proc(mario_layer, &mario_update_callback);
    layer_set_update_proc(ground_layer, &ground_update_callback);
		layer_set_update_proc(no_phone_layer, &no_phone_update_callback);
		layer_set_update_proc(battery_layer, &battery_update_callback);

    Layer *window_layer = window_get_root_layer(window);

    layer_add_child(window_layer, blocks_layer);
    layer_add_child(window_layer, mario_layer);
    layer_add_child(window_layer, ground_layer);
		layer_add_child(window_layer, no_phone_layer);
		layer_add_child(window_layer, battery_layer);

    text_hour_layer = text_layer_create(hour_normal_rect);
    text_layer_set_background_color(text_hour_layer, GColorClear);
    text_layer_set_text_alignment(text_hour_layer, GTextAlignmentCenter);
    text_layer_set_font(text_hour_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(blocks_layer, (Layer *)text_hour_layer);

    text_minute_layer = text_layer_create(GRect(55, 5, 40, 40));
    text_layer_set_background_color(text_minute_layer, GColorClear);
    text_layer_set_text_alignment(text_minute_layer, GTextAlignmentCenter);
    text_layer_set_font(text_minute_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(blocks_layer, (Layer *)text_minute_layer);

    GRect date_rect = GRect(30, 6, 144-30*2, ground_rect.size.h-6*2);
    date_layer = text_layer_create(date_rect);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(ground_layer, (Layer *)date_layer);

		update_colors();
		load_bitmaps();

#if defined(DEMO)
    #define MARIO_TIME_UNIT SECOND_UNIT
#else
    #define MARIO_TIME_UNIT MINUTE_UNIT
#endif
    tick_timer_service_subscribe(MARIO_TIME_UNIT, handle_tick);
		
		bluetooth_connection_service_subscribe(bluetooth_connection_callback);
		battery_state_service_subscribe(handle_battery);
}

void handle_deinit()
{
    tick_timer_service_unsubscribe();

    animation_unschedule_all();

    property_animation_destroy(mario_animation_beg);
    property_animation_destroy(mario_animation_end);
    property_animation_destroy(block_animation_beg);
    property_animation_destroy(block_animation_end);
    property_animation_destroy(hour_animation_slide_in);
    property_animation_destroy(hour_animation_slide_away);
    property_animation_destroy(minute_animation_slide_in);
    property_animation_destroy(minute_animation_slide_away);

    gbitmap_destroy(mario_normal_bmp);
    gbitmap_destroy(mario_jump_bmp);
    gbitmap_destroy(ground_bmp);
		gbitmap_destroy(no_phone_bmp);
		gbitmap_destroy(battery_bmp);
		gbitmap_destroy(battery_charging_bmp);		

    text_layer_destroy(date_layer);
    text_layer_destroy(text_minute_layer);
    text_layer_destroy(text_hour_layer);

    layer_destroy(ground_layer);
    layer_destroy(mario_layer);
    layer_destroy(blocks_layer);
		layer_destroy(no_phone_layer);
		layer_destroy(battery_layer);

    window_destroy(window);
		
		bluetooth_connection_service_unsubscribe();
		battery_state_service_unsubscribe();
		app_message_deregister_callbacks();
}

void mario_down_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
}

void mario_down_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
    mario_is_down = 1;
    layer_mark_dirty(mario_layer);
}

void mario_jump_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
    mario_is_down = 0;
    layer_mark_dirty(mario_layer);
}

void mario_jump_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    text_layer_set_text(text_hour_layer, hour_text);
    text_layer_set_text(text_minute_layer, minute_text);

    if (!mario_animation_end) {
        mario_animation_end = property_animation_create_layer_frame(mario_layer,
                                                                    &mario_up_rect,
                                                                    &mario_down_rect);
        animation_set_duration((Animation *)mario_animation_end, MARIO_JUMP_DURATION);
        animation_set_curve((Animation *)mario_animation_end, AnimationCurveEaseIn);
        animation_set_handlers((Animation *)mario_animation_end, (AnimationHandlers){
            .started = (AnimationStartedHandler)mario_down_animation_started,
            .stopped = (AnimationStoppedHandler)mario_down_animation_stopped
        }, 0);
    }

    animation_schedule((Animation *)mario_animation_end);
}

void block_up_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
}

void block_up_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    if (!block_animation_end) {
        block_animation_end = property_animation_create_layer_frame(blocks_layer,
                                                                    &blocks_up_rect,
                                                                    &blocks_down_rect);
        animation_set_duration((Animation *)block_animation_end, MARIO_JUMP_DURATION);
        animation_set_curve((Animation *)block_animation_end, AnimationCurveEaseIn);
    }
    animation_schedule((Animation *)block_animation_end);
}

void clock_animation_slide_away_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
}

void clock_animation_slide_away_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    layer_set_frame((Layer *)text_hour_layer, hour_down_rect);
    layer_set_frame((Layer *)text_minute_layer, minute_down_rect);

    if (!hour_animation_slide_in) {
        hour_animation_slide_in = property_animation_create_layer_frame((Layer *)text_hour_layer,
                                                                        &hour_down_rect,
                                                                        &hour_normal_rect);
        animation_set_duration((Animation *)hour_animation_slide_in, CLOCK_ANIMATION_DURATION);
        animation_set_curve((Animation *)hour_animation_slide_in, AnimationCurveLinear);
    }

    if (!minute_animation_slide_in) {
        minute_animation_slide_in = property_animation_create_layer_frame((Layer *)text_minute_layer,
                                                                          &minute_down_rect,
                                                                          &minute_normal_rect);
        animation_set_duration((Animation *)minute_animation_slide_in, CLOCK_ANIMATION_DURATION);
        animation_set_curve((Animation *)minute_animation_slide_in, AnimationCurveLinear);
    }

    animation_schedule((Animation *)hour_animation_slide_in);
    animation_schedule((Animation *)minute_animation_slide_in);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
    if (!mario_animation_beg) {
        mario_animation_beg = property_animation_create_layer_frame(mario_layer,
                                                                    &mario_down_rect,
                                                                    &mario_up_rect);
        animation_set_duration((Animation *)mario_animation_beg, MARIO_JUMP_DURATION);
        animation_set_curve((Animation *)mario_animation_beg, AnimationCurveEaseOut);
        animation_set_handlers((Animation *)mario_animation_beg, (AnimationHandlers){
            .started = (AnimationStartedHandler)mario_jump_animation_started,
            .stopped = (AnimationStoppedHandler)mario_jump_animation_stopped
        }, 0);
    }

    if (!block_animation_beg) {
        block_animation_beg = property_animation_create_layer_frame(blocks_layer,
                                                                    &blocks_down_rect,
                                                                    &blocks_up_rect);
        animation_set_duration((Animation *)block_animation_beg, MARIO_JUMP_DURATION);
        animation_set_curve((Animation *)block_animation_beg, AnimationCurveEaseOut);
        animation_set_handlers((Animation *)block_animation_beg, (AnimationHandlers){
            .started = (AnimationStartedHandler)block_up_animation_started,
            .stopped = (AnimationStoppedHandler)block_up_animation_stopped
        }, 0);
    }

    if (!hour_animation_slide_away) {
        hour_animation_slide_away = property_animation_create_layer_frame((Layer *)text_hour_layer,
                                                                          &hour_normal_rect,
                                                                          &hour_up_rect);
        animation_set_duration((Animation *)hour_animation_slide_away, CLOCK_ANIMATION_DURATION);
        animation_set_curve((Animation *)hour_animation_slide_away, AnimationCurveLinear);
        animation_set_handlers((Animation *)hour_animation_slide_away, (AnimationHandlers){
            .started = (AnimationStartedHandler)clock_animation_slide_away_started,
            .stopped = (AnimationStoppedHandler)clock_animation_slide_away_stopped
        }, 0);
    }

    if (!minute_animation_slide_away) {
        minute_animation_slide_away = property_animation_create_layer_frame((Layer *)text_minute_layer,
                                                                            &minute_normal_rect,
                                                                            &minute_up_rect);
        animation_set_duration((Animation *)minute_animation_slide_away, CLOCK_ANIMATION_DURATION);
        animation_set_curve((Animation *)minute_animation_slide_away, AnimationCurveLinear);
    }

#if defined(DEMO)
    strncpy(date_text, "Sat, May 19", sizeof(date_text)-1);
    date_text[sizeof(date_text)-1] = '\0';

    hour_text[0] = '9';
    hour_text[1] = '\0';
    minute_text[0] = '4';
    minute_text[1] = '1';
    minute_text[2] = '\0';
    if (demo_advance_time) {
        minute_text[1] = '2';
    }
    demo_advance_time ^= 1;
#else
    char *hour_format;
    if (clock_is_24h_style()) {
        hour_format = "%H";
    } else {
        hour_format = "%I";
    }

    strftime(hour_text, sizeof(hour_text), hour_format, tick_time);
    if (!clock_is_24h_style() && (hour_text[0] == '0')) {
        memmove(hour_text, &hour_text[1], sizeof(hour_text) - 1);
    }

    char *minute_format = "%M";
    strftime(minute_text, sizeof(minute_text), minute_format, tick_time);

    strftime(date_text, sizeof(date_text), "%a, %b %d", tick_time);
#endif

    animation_schedule((Animation *)mario_animation_beg);
    animation_schedule((Animation *)block_animation_beg);
    animation_schedule((Animation *)hour_animation_slide_away);
    animation_schedule((Animation *)minute_animation_slide_away);
}

int main(void)
{
    handle_init();
    app_event_loop();
    handle_deinit();
}
