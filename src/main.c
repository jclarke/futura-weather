#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "http.h"
#include "util.h"
#include "weather_layer.h"
#include "time_layer.h"
#include "config.h"

#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x04, 0x9F, 0x49, 0xC0, 0x99, 0xAD }

PBL_APP_INFO(MY_UUID,
             "Pebble Status Board", "Joe Clarke", // Modification of Futura Weather by Niknam Moslehi // Modification of "Roboto Weather" by Martin Rosinski
             1, 01, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(1, 65, 144, 168-62))

// POST variables
#define WEATHER_KEY_LATITUDE 1
#define WEATHER_KEY_LONGITUDE 2
#define WEATHER_KEY_UNIT_SYSTEM 3
	
// Received variables
#define WEATHER_KEY_ICON 1
#define WEATHER_KEY_TEMPERATURE 2
#define EMAIL_KEY_UNREAD 3
#define SEND_VIBRATE 4
#define ACTIVATION_CODE 5
	
#define WEATHER_HTTP_COOKIE 1949327679
#define TIME_HTTP_COOKIE 1131038289

Window window;          /* main window */
TextLayer date_layer;   /* layer for the date */
TimeLayer time_layer;   /* layer for the time */

GFont font_date;        /* font for date */
GFont font_hour;        /* font for hour */
GFont font_minute;      /* font for minute */

//Weather Stuff
static int our_latitude, our_longitude, failed_count = 0, random_number = 0;
static bool located = false;

WeatherLayer weather_layer;

void request_data();

void failed(int32_t cookie, int http_status, void* context) {
	failed_count = failed_count + 1;
	if (failed_count > 10) {
	  weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
	}
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	if(cookie != WEATHER_HTTP_COOKIE) return;
	failed_count = 0;
	/*
	Tuple* icon_tuple = dict_find(received, WEATHER_KEY_ICON);
	if(icon_tuple) {
		int icon = icon_tuple->value->int8;
		if(icon >= 0 && icon < 10) {
			weather_layer_set_icon(&weather_layer, icon);
		} else {
			weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		}
	}
	*/
	Tuple* activation_tuple = dict_find(received, ACTIVATION_CODE);
	
	if (activation_tuple) {
	   weather_layer_set_activation_code(&weather_layer, activation_tuple->value->int16);	
    }
    else {
  	  Tuple* temperature_tuple = dict_find(received, WEATHER_KEY_TEMPERATURE);
  	  if(temperature_tuple) {
		weather_layer_set_temperature(&weather_layer, temperature_tuple->value->int16);
	  }
	  Tuple* email_tuple = dict_find(received, EMAIL_KEY_UNREAD);
	  Tuple* vibrate_tuple = dict_find(received, SEND_VIBRATE);
	  if (email_tuple) {
	    weather_layer_set_unread_messages(&weather_layer, email_tuple->value->int16, vibrate_tuple->value->int16);
	  }
    }
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
	// Fix the floats
	our_latitude = latitude * 10000;
	our_longitude = longitude * 10000;
	located = true;
	request_data();
}

void reconnect(void* context) {
	located = false;
	request_data();
}

void request_data();

/* Called by the OS once per minute. Update the time and date.
*/
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    /* Need to be static because pointers to them are stored in the text
    * layers.
    */
    static char date_text[] = "XXX, XXX 00";
    static char hour_text[] = "00";
    static char minute_text[] = ":00";

    (void)ctx;  /* prevent "unused parameter" warning */

    if (t->units_changed & DAY_UNIT)
    {		
	    string_format_time(date_text,
                           sizeof(date_text),
                           "%a, %b %d",
                           t->tick_time);

        
		
        text_layer_set_text(&date_layer, date_text);
    }

    if (clock_is_24h_style())
    {
        string_format_time(hour_text, sizeof(hour_text), "%H", t->tick_time);
		if (hour_text[0] == '0')
        {
            /* This is a hack to get rid of the leading zero of the hour.
            */
            memmove(&hour_text[0], &hour_text[1], sizeof(hour_text) - 1);
        }
    }
    else
    {
        string_format_time(hour_text, sizeof(hour_text), "%I", t->tick_time);
        if (hour_text[0] == '0')
        {
            /* This is a hack to get rid of the leading zero of the hour.
            */
            memmove(&hour_text[0], &hour_text[1], sizeof(hour_text) - 1);
        }
    }

    string_format_time(minute_text, sizeof(minute_text), ":%M", t->tick_time);
    time_layer_set_text(&time_layer, hour_text, minute_text);

	if(!located || !(t->tick_time->tm_min % 15)) {
	   //Every 15 minutes, update location
	   http_location_request();
	}
	else {
	  request_data(t->tick_time->tm_min);
	}
	
}


/* Initialize the application.
*/
void handle_init(AppContextRef ctx)
{
    PblTm tm;
    PebbleTickEvent t;
    ResHandle res_d;
    ResHandle res_h;
    ResHandle res_m;

    window_init(&window, "Futura");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);

    resource_init_current_app(&APP_RESOURCES);

    res_d = resource_get_handle(RESOURCE_ID_FUTURA_18);
    res_h = resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53);

    font_date = fonts_load_custom_font(res_d);
    font_hour = fonts_load_custom_font(res_h);
    font_minute = fonts_load_custom_font(res_h);
    
    // Time Display
    time_layer_init(&time_layer, window.layer.frame);
    time_layer_set_text_color(&time_layer, GColorWhite);
    time_layer_set_background_color(&time_layer, GColorClear);
    time_layer_set_fonts(&time_layer, font_hour, font_minute);
    layer_set_frame(&time_layer.layer, TIME_FRAME);
    layer_add_child(&window.layer, &time_layer.layer);
    
    // Date Display
    text_layer_init(&date_layer, window.layer.frame);
    text_layer_set_text_color(&date_layer, GColorWhite);
    text_layer_set_background_color(&date_layer, GColorClear);
    text_layer_set_font(&date_layer, font_date);
    text_layer_set_text_alignment(&date_layer, GTextAlignmentCenter);
    layer_set_frame(&date_layer.layer, DATE_FRAME);
    layer_add_child(&window.layer, &date_layer.layer);

	// Status Board Display
	weather_layer_init(&weather_layer, GPoint(0, 90));
	layer_add_child(&window.layer, &weather_layer.layer);
	
	http_register_callbacks((HTTPCallbacks){.failure=failed,.success=success,.reconnect=reconnect,.location=location}, (void*)ctx);
	
	// Refresh time
	get_time(&tm);
    t.tick_time = &tm;
    t.units_changed = SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT;
	handle_minute_tick(ctx, &t);
}

/* Shut down the application
*/
void handle_deinit(AppContextRef ctx)
{
    fonts_unload_custom_font(font_date);
    fonts_unload_custom_font(font_hour);
    fonts_unload_custom_font(font_minute);
	
	weather_layer_deinit(&weather_layer);
}


/********************* Main Program *******************/

void pbl_main(void *params)
{
    PebbleAppHandlers handlers =
    {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
        .tick_info =
        {
            .tick_handler = &handle_minute_tick,
            .tick_units = MINUTE_UNIT
        },
		.messaging_info = {
			.buffer_sizes = {
				.inbound = 124,
				.outbound = 256,
			}
		}
    };

    app_event_loop(params, &handlers);
}

void request_data() {
	if (!located) {
	  http_location_request();
	  return;
	}
	char * time_text;
	time_text = itoa(time(NULL));
	char * url = "http://serverping.net:3001/get_data/";
	//url = "http://serverping.net:3001/get_data/";
	strcpy(url,"http://serverping.net:3001/get_data/");
	char * url1 = strcat(url, time_text);
	DictionaryIterator *body;
	HTTPResult result = http_out_get(url1,WEATHER_HTTP_COOKIE, &body);
	if (result != HTTP_OK) {
      weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
	  return;
	}
	
	dict_write_int32(body, WEATHER_KEY_LATITUDE, our_latitude);
	dict_write_int32(body, WEATHER_KEY_LONGITUDE, our_longitude);
	dict_write_cstring(body, WEATHER_KEY_UNIT_SYSTEM, UNIT_SYSTEM);
	
	if (http_out_send() != HTTP_OK) {
	  weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
	  return;
	}
}