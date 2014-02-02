#include <pebble.h>

#define INVERT_COLORS  0

#ifndef INVERT_COLORS
#define INVERT_COLORS  1
#define COLOR_FOREGROUND GColorWhite
#define COLOR_BACKGROUND GColorBlack
#else
#define COLOR_FOREGROUND GColorBlack
#define COLOR_BACKGROUND GColorWhite
#endif

#define DEBUG_MODE 0
#define PULSE_MODE 1

#define LINE_LEVEL 80 // height of marker live.
#define GRADIENT 4 // distance each 5 min line apart

#define FUDGE      28  // changing the gradient should automatically adjust the scroll offself correctly
                       // but it's leaving the screen too far down - move it back up by this much
                       // (I can't see why - the maths here seems pretty straight forward...)

//#define GRADIENT 3 // distance each 5 min line apart
//#define FUDGE      0




Window *window;
Layer *rootLayer;
Layer *rulerLayer; // The board/grid
Layer *lineLayer; // The board/grid
Layer *bgLayer;   // the background

TextLayer *hourLayers[30];
char hourStrings[30][13];

void init_hours() {
	GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
	for (int _hour = 0; _hour < 30; _hour++) {
	        int display_hour;

		// 12 gradients per hour, subtract 5 to make the number roughly in the middle of the line
		hourLayers[_hour] = text_layer_create(GRect(70, (_hour * (12 * GRADIENT)) - 15 ,40,30));
		text_layer_set_font(hourLayers[_hour], font);
		text_layer_set_background_color(hourLayers[_hour], GColorClear);
		text_layer_set_text_color(hourLayers[_hour], COLOR_FOREGROUND);

		// we are displaying a rolling frame of 29 odd hour markers (to make sure
		// we have extra numbers at the start and end to facillate rollinng around
		// as we are displaying this two hours backwards (to make the time marker
		// in the centre of the screen, we need to adjust the hour - make sure
		// it's less than 24, and make sure it's greeater than one
		// it doesn't display as -1 or -2, or 27 o'clock
		display_hour = (_hour - 2) % 24;
		if (display_hour < 0) display_hour = display_hour + 24;
			
		// convert to 12h format if that's what the phone is set to
		if (!clock_is_24h_style()) {
		  display_hour = (display_hour % 12);
		  if (display_hour == 0) _hour = 12;
		}
		snprintf(hourStrings[_hour], 12, "%d", display_hour);
		text_layer_set_text(hourLayers[_hour], hourStrings[_hour]);

		layer_add_child(rulerLayer, text_layer_get_layer(hourLayers[_hour]));
	}
}

void deinit_hours() {
	int i;
	for (i = 0; i < 30; i++) {
		text_layer_destroy(hourLayers[i]);
	}
}


// draws the current time line marker
void lineLayer_update_callback (Layer *me, GContext* ctx) {
	int offset = 120;

	graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);
	graphics_draw_line(ctx, GPoint(0, offset), GPoint(144, offset));
	offset++;
	graphics_draw_line(ctx, GPoint(0, offset), GPoint(144, offset));
}


void bgLayer_update_callback(Layer *layer, GContext* ctx) {
	int y = LINE_LEVEL; // position of marker line
						// when inverting, we don't want the inset color, as otherwise it would be
						// black (plastic), white (screen), black (screen). Just make it all black
	if (INVERT_COLORS) {
		graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
	} else {
		graphics_context_set_fill_color(ctx, COLOR_FOREGROUND);
		//graphics_fill_rect(ctx, GRect(0,0,144, 168), 0, GCornersAll);
		graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
		graphics_context_set_fill_color(ctx, GColorClear);
		graphics_fill_rect(ctx, GRect(5,5,144 - 10, 168 - 10) , 4, GCornersAll);
		
	}
	
	// draw the time marker line
	graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);
	graphics_draw_line(ctx, GPoint(0, y), GPoint(144, y));
	graphics_draw_line(ctx, GPoint(0, y+1), GPoint(144, y+1));
}

void drawRuler(GContext *ctx) {
	int x = 0;
	int y = 0;
	
	graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);

	// draw 29 hours worth of lines as we need to be able to have 23.59 get to the top of the screen
	// whilst still having the next few hours beneath it. (so hours 0-3 are duplicated)
	for (int _hour = 0 ; _hour < 30 ; _hour++ ) {
		for (int _min = 0; _min < 59; _min= _min + 5 ) {
			y = y + GRADIENT;
			if  (_min  == 0)  {
				x =  60;
			} else if  (_min % 30 == 0 ) {
				x = 50;
			} else if  (_min % 15 == 0 ) {
				x = 40;
			} else {
				x = 30;
			}
			
			graphics_draw_line(ctx, GPoint(19, y), GPoint(x, y));
		}
	}
}


void rulerLayer_move_current_offset () {
	time_t t;
	struct tm *now;

	t = time(NULL);
	now = localtime(&t);

	int total_mins = ((now->tm_hour * 60) + now->tm_min);
	int offset = ((total_mins / 5) * GRADIENT * -1) - FUDGE ;

	// offset the bounds of the layer by the length needed to show the current 
	// time
	layer_set_bounds(rulerLayer, GRect(0, offset ,100 ,100));
}

void rulerLayer_update_callback (Layer *me, GContext* ctx) {
        rulerLayer_move_current_offset();
	drawRuler(ctx);
}

void init_line_layer() {
	lineLayer = layer_create(layer_get_frame(rootLayer)); // Associate with layer object and set dimensions
	if (!INVERT_COLORS) {
		layer_set_update_proc(lineLayer, lineLayer_update_callback);
	}
	 // Set the drawing callback function for the layer.
	layer_add_child(rootLayer, lineLayer); // Add the child to the app's base window
}

void init_bg_layer() {
	bgLayer = layer_create(layer_get_frame(rootLayer)); // Associate with layer object and set dimensions
	layer_set_update_proc(bgLayer, bgLayer_update_callback); // Set the drawing callback function for the layer.
	layer_add_child(rootLayer, bgLayer); // Add the child to the app's base window
}

void init_ruler_layer() {
	rulerLayer = layer_create(layer_get_frame(rootLayer)); // Associate with layer object and set dimensions
	layer_set_update_proc(rulerLayer, rulerLayer_update_callback); // Set the drawing callback function for the layer.

	//layer_set_frame(&rulerLayer, GRect(0, offset ,144  ,148));
	//set the frame to be the area on the screen that we want the 
	//ruler lines to be visible in (this has clipping off so we wont see
	//anything outside this box)
	layer_set_frame(rulerLayer, GRect(5, 5 ,144-20  ,168-10));

        rulerLayer_move_current_offset();

	layer_add_child(rootLayer, rulerLayer); // Add the child to the app's base window
}

void deinit_layers() {
	layer_destroy(rulerLayer);
	layer_destroy(bgLayer);
	layer_destroy(lineLayer);
}


// once a minute update position of the ruler on the screen
void handle_tick(struct tm *now, TimeUnits units_changed) {
	if (PULSE_MODE && now->tm_min == 0) {
		vibes_short_pulse();
	}

	layer_mark_dirty(rootLayer);
}


void handle_init() {
	window = window_create();
	window_set_background_color(window, COLOR_BACKGROUND);
	window_stack_push(window, true);

	rootLayer = window_get_root_layer(window);

	init_line_layer();
	init_bg_layer();
	init_ruler_layer();
	init_hours();

	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}


void handle_deinit() {
	tick_timer_service_unsubscribe();
	deinit_hours();
	deinit_layers();
	window_destroy(window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
