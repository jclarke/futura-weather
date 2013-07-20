#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "util.h"
#include "weather_layer.h"

static uint8_t WEATHER_ICONS[] = {
	RESOURCE_ID_ICON_CLEAR_DAY,
	RESOURCE_ID_ICON_CLEAR_NIGHT,
	RESOURCE_ID_ICON_RAIN,
	RESOURCE_ID_ICON_SNOW,
	RESOURCE_ID_ICON_SLEET,
	RESOURCE_ID_ICON_WIND,
	RESOURCE_ID_ICON_FOG,
	RESOURCE_ID_ICON_CLOUDY,
	RESOURCE_ID_ICON_PARTLY_CLOUDY_DAY,
	RESOURCE_ID_ICON_PARTLY_CLOUDY_NIGHT,
	RESOURCE_ID_ICON_ERROR,
};

void weather_layer_init(WeatherLayer* weather_layer, GPoint pos) {
	layer_init(&weather_layer->layer, GRect(pos.x, pos.y, 144, 80));
	
	// Add background layer
	text_layer_init(&weather_layer->temp_layer_background, GRect(0, 10, 144, 68));
	text_layer_set_background_color(&weather_layer->temp_layer_background, GColorWhite);
	layer_add_child(&weather_layer->layer, &weather_layer->temp_layer_background.layer);
	
    // Add temperature layer
	text_layer_init(&weather_layer->temp_layer, GRect(70, 19, 72, 80));
	text_layer_set_background_color(&weather_layer->temp_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->temp_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40)));
	layer_add_child(&weather_layer->layer, &weather_layer->temp_layer.layer);
    
    // Unread Email Messages Layer
	text_layer_init(&weather_layer->messages_layer, GRect(40,19, 40, 40));
	text_layer_set_background_color(&weather_layer->messages_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->messages_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->messages_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35)));
	layer_add_child(&weather_layer->layer, &weather_layer->messages_layer.layer);	

    // Unread Facebook Messages/Notifications Layer
	text_layer_init(&weather_layer->facebook_messages_layer, GRect(40, 50, 40, 40));
	text_layer_set_background_color(&weather_layer->facebook_messages_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->facebook_messages_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->facebook_messages_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35)));
	layer_add_child(&weather_layer->layer, &weather_layer->facebook_messages_layer.layer);	


    // Activation Code Layer
	text_layer_init(&weather_layer->activation_code_layer, GRect(10, 10, 100, 100));
	text_layer_set_background_color(&weather_layer->activation_code_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->activation_code_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->activation_code_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35)));

	weather_layer->has_weather_icon = false;
	weather_layer->has_no_link_icon = false;
	weather_layer->has_mail_icon = false;
	weather_layer->has_facebook_icon = false;
	weather_layer->has_activation_code = false;
    weather_layer->unread_messages = 0;
}

void weather_layer_set_no_link_icon(WeatherLayer* weather_layer) {
	
	if (weather_layer->has_mail_icon) {
		layer_remove_from_parent(&weather_layer->icon_layer.layer.layer);
		bmp_deinit_container(&weather_layer->icon_layer);
		weather_layer->has_mail_icon = false;
	}
	
	if (weather_layer->has_facebook_icon) {
	   layer_remove_from_parent(&weather_layer->icon_layer2.layer.layer);
	   bmp_deinit_container(&weather_layer->icon_layer2);
	   weather_layer->has_facebook_icon = false;
	}
	
	
	if (!weather_layer->has_no_link_icon) {
  	  bmp_init_container(RESOURCE_ID_ICON_ERROR, &weather_layer->icon_layer);
	  layer_add_child(&weather_layer->layer, &weather_layer->icon_layer.layer.layer);
	  layer_set_frame(&weather_layer->icon_layer.layer.layer, GRect(9, 13, 60, 60));
      weather_layer->has_no_link_icon = true;
      weather_layer->has_mail_icon = false; 
    }
}

void weather_layer_set_weather_icon(WeatherLayer* weather_layer, WeatherIcon icon) {
	
	if(weather_layer->has_weather_icon) {
		layer_remove_from_parent(&weather_layer->icon_layer3.layer.layer);
		bmp_deinit_container(&weather_layer->icon_layer3);
		weather_layer->has_weather_icon = false;
	}
	
	// Add icon
	bmp_init_container(WEATHER_ICONS[icon], &weather_layer->icon_layer3);
	layer_add_child(&weather_layer->layer, &weather_layer->icon_layer3.layer.layer);
	layer_set_frame(&weather_layer->icon_layer3.layer.layer, GRect(80, 45, 30, 30));
	weather_layer->has_weather_icon = true;
}

void weather_layer_set_temperature(WeatherLayer* weather_layer, int16_t t) {
	memcpy(weather_layer->temp_str, itoa(t), 4);
	int degree_pos = strlen(weather_layer->temp_str);
	
	if (strlen(weather_layer->temp_str) == 1 || 
		(strlen(weather_layer->temp_str) == 2 && weather_layer->temp_str[0] != '1')) {
	  // Don't move temperature if between 0-9° or 20°-99°
	  text_layer_set_font(&weather_layer->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->temp_layer, GTextAlignmentCenter);
	  memcpy(&weather_layer->temp_str[degree_pos], "°", 3);
	} else if (strlen(weather_layer->temp_str) == 2 && weather_layer->temp_str[0] == '1') {
	  // Move temperature slightly to the left if between 10°-19°
	  text_layer_set_font(&weather_layer->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->temp_layer, GTextAlignmentLeft);
	  memcpy(&weather_layer->temp_str[degree_pos], "°", 3); 
	} else if (strlen(weather_layer->temp_str) > 2) { 
	  // Shrink font size if above 99° or below -9°
	  text_layer_set_font(&weather_layer->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->temp_layer, GTextAlignmentCenter);
	  memcpy(&weather_layer->temp_str[degree_pos], "°", 3);
	}
	
	text_layer_set_text(&weather_layer->temp_layer, weather_layer->temp_str);
}

void weather_layer_set_activation_code(WeatherLayer* weather_layer, char code[4]) {
	if (weather_layer->has_mail_icon) {
		layer_remove_from_parent(&weather_layer->icon_layer.layer.layer);
		bmp_deinit_container(&weather_layer->icon_layer);
		weather_layer->has_mail_icon = false;
	}
	
	if (weather_layer->has_facebook_icon) {
	   layer_remove_from_parent(&weather_layer->icon_layer2.layer.layer);
	   bmp_deinit_container(&weather_layer->icon_layer2);
	   weather_layer->has_facebook_icon = false;
	}
	
	if (!weather_layer->has_activation_code) {
      layer_add_child(&weather_layer->layer, &weather_layer->activation_code_layer.layer);	
    }
    memcpy(weather_layer->activation_code, code, 4);
	text_layer_set_font(&weather_layer->activation_code_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35)));
	text_layer_set_text_alignment(&weather_layer->activation_code_layer, GTextAlignmentLeft);
	text_layer_set_text(&weather_layer->activation_code_layer, weather_layer->activation_code);
	weather_layer->has_activation_code = true;
}

void weather_layer_set_unread_facebook_messages(WeatherLayer* weather_layer, int16_t m) {
	if (weather_layer->has_activation_code) {
	  weather_layer->has_activation_code = false;
	  layer_remove_from_parent(&weather_layer->activation_code_layer.layer);	
	}
	
	if (m != 0) {
	  if(!weather_layer->has_facebook_icon) {
        // Add icon
	    bmp_init_container(RESOURCE_ID_ICON_FACEBOOK, &weather_layer->icon_layer2);
	    layer_add_child(&weather_layer->layer, &weather_layer->icon_layer2.layer.layer);
	    layer_set_frame(&weather_layer->icon_layer2.layer.layer, GRect(10, 50, 20, 20));
	    weather_layer->has_facebook_icon = true;
	  }
	  weather_layer->unread_facebook_messages = m;
	  memcpy(weather_layer->facebook_messages_str, itoa(m), 4);
	  text_layer_set_font(&weather_layer->facebook_messages_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->facebook_messages_layer, GTextAlignmentLeft);
	  text_layer_set_text(&weather_layer->facebook_messages_layer, weather_layer->facebook_messages_str);	 
    }
    else {
	  if(!weather_layer->has_facebook_icon) {
        // Add icon
	    bmp_init_container(RESOURCE_ID_ICON_FACEBOOK, &weather_layer->icon_layer2);
	    layer_add_child(&weather_layer->layer, &weather_layer->icon_layer2.layer.layer);
	    layer_set_frame(&weather_layer->icon_layer2.layer.layer, GRect(10, 50, 20, 20));
	    weather_layer->has_facebook_icon = true;
      }	
	  weather_layer->unread_facebook_messages = 0;
	  memcpy(weather_layer->facebook_messages_str, "0", 4);
	  text_layer_set_font(&weather_layer->facebook_messages_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->facebook_messages_layer, GTextAlignmentLeft);
	  text_layer_set_text(&weather_layer->facebook_messages_layer, weather_layer->facebook_messages_str);
    }
}

void weather_layer_set_unread_messages(WeatherLayer* weather_layer, int16_t m) {
	if (weather_layer->has_activation_code) {
		weather_layer->has_activation_code = false;
	  layer_remove_from_parent(&weather_layer->activation_code_layer.layer);	
	}
	
	if (weather_layer->has_no_link_icon) {
		layer_remove_from_parent(&weather_layer->icon_layer.layer.layer);
		bmp_deinit_container(&weather_layer->icon_layer);
		weather_layer->has_no_link_icon = false;
	}
	
	if (m != 0) {
	  if (!weather_layer->has_mail_icon) {
	    bmp_init_container(RESOURCE_ID_ICON_EMAIL, &weather_layer->icon_layer);
        layer_add_child(&weather_layer->layer, &weather_layer->icon_layer.layer.layer);
  	    layer_set_frame(&weather_layer->icon_layer.layer.layer, GRect(10, 19, 20, 20));
	    weather_layer->has_mail_icon = true;
	  }
	  weather_layer->unread_messages = m;
	  memcpy(weather_layer->messages_str, itoa(m), 4);
	  text_layer_set_font(&weather_layer->messages_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->messages_layer, GTextAlignmentLeft);
	  text_layer_set_text(&weather_layer->messages_layer, weather_layer->messages_str);	 
    }
    else {
	  if (!weather_layer->has_mail_icon) {
	    bmp_init_container(RESOURCE_ID_ICON_EMAIL, &weather_layer->icon_layer);
        layer_add_child(&weather_layer->layer, &weather_layer->icon_layer.layer.layer);
  	    layer_set_frame(&weather_layer->icon_layer.layer.layer, GRect(10, 19, 20, 20));
	    weather_layer->has_mail_icon = true;
	  }
	  weather_layer->unread_messages = 0;
	  memcpy(weather_layer->messages_str, "0", 4);
	  text_layer_set_font(&weather_layer->messages_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18)));
	  text_layer_set_text_alignment(&weather_layer->messages_layer, GTextAlignmentLeft);
	  text_layer_set_text(&weather_layer->messages_layer, weather_layer->messages_str);
    }
}

void weather_layer_deinit(WeatherLayer* weather_layer) {
	if (weather_layer->has_mail_icon || weather_layer->has_no_link_icon)
		bmp_deinit_container(&weather_layer->icon_layer);
	if (weather_layer->has_facebook_icon)
	   bmp_deinit_container(&weather_layer->icon_layer2);
	if (weather_layer->has_weather_icon)
		bmp_deinit_container(&weather_layer->icon_layer3);
}