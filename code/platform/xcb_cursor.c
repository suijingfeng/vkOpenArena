#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <xcb/xcb.h>

#define WIDTH 300 
#define HEIGHT 150 

static void testCookie(xcb_void_cookie_t, xcb_connection_t*, char *); 
static void drawButton(xcb_connection_t*, xcb_screen_t*, xcb_window_t, int16_t, int16_t, const char*);
static void drawText(xcb_connection_t*, xcb_screen_t*, xcb_window_t, int16_t, int16_t, const char*);
static xcb_gc_t getFontGC(xcb_connection_t*, xcb_screen_t*, xcb_window_t, const char*);
static void setCursor (xcb_connection_t*, xcb_screen_t*, xcb_window_t, int);

/*  
*/  
static void testCookie (
		xcb_void_cookie_t cookie, 
		xcb_connection_t *connection,
		char *errMessage )
{   
	xcb_generic_error_t *error = xcb_request_check (connection, cookie);
	if (error) {
		fprintf (stderr, "ERROR: %s : %"PRIu8"\n", errMessage , error->error_code);
		xcb_disconnect (connection);
		exit (-1);
	}   
}   

/*  
*/  
static void drawButton (xcb_connection_t *connection,
		xcb_screen_t     *screen,
		xcb_window_t      window,
		int16_t           x1, 
		int16_t           y1, 
		const char       *label )
{   
	uint8_t length = strlen (label);
	int16_t inset = 2;
	int16_t width = 7 * length + 2 * (inset + 1); 
	int16_t height = 13 + 2 * (inset + 1); 

	xcb_point_t points[5];
	points[0].x = x1; 
	points[0].y = y1; 
	points[1].x = x1 + width;
	points[1].y = y1; 
	points[2].x = x1 + width;
	points[2].y = y1 - height;
	points[3].x = x1; 
	points[3].y = y1 - height;
	points[4].x = x1; 
	points[4].y = y1; 

	xcb_gcontext_t gc = getFontGC (connection, screen, window, "fixed");
	xcb_void_cookie_t lineCookie = xcb_poly_line_checked (connection,
			XCB_COORD_MODE_ORIGIN,
			window,
			gc,
			5,
			points );
	testCookie (lineCookie, connection, "can't draw lines");

	xcb_void_cookie_t textCookie = xcb_image_text_8_checked (connection,
			length,
			window,
			gc,
			x1 + inset + 1,
			y1 - inset - 1,
			label );
	testCookie (textCookie, connection, "can't paste text");

	xcb_void_cookie_t gcCookie = xcb_free_gc (connection, gc);
	testCookie (gcCookie, connection, "can't free gc");
}

/*
*/
	static void
drawText (xcb_connection_t *connection,
		xcb_screen_t     *screen,
		xcb_window_t      window,
		int16_t           x1,
		int16_t           y1,
		const char       *label )
{

	xcb_gcontext_t gc = getFontGC (connection, screen, window, "fixed");
	xcb_void_cookie_t textCookie = xcb_image_text_8_checked (connection,
			strlen (label),
			window,
			gc,
			x1,
			y1,
			label );
	testCookie(textCookie, connection, "can't paste text");

	xcb_void_cookie_t gcCookie = xcb_free_gc (connection, gc);
	testCookie (gcCookie, connection, "can't free gc");
}

/*
*/
	static xcb_gc_t
getFontGC (xcb_connection_t *connection,
		xcb_screen_t     *screen,
		xcb_window_t      window,
		const char       *fontName )
{

	xcb_font_t font = xcb_generate_id (connection);
	xcb_void_cookie_t fontCookie = xcb_open_font_checked (connection,
			font,
			strlen (fontName),
			fontName );
	testCookie (fontCookie, connection, "can't open font");

	xcb_gcontext_t gc = xcb_generate_id (connection);
	uint32_t  mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	uint32_t value_list[3];
	value_list[0] = screen->black_pixel;
	value_list[1] = screen->white_pixel;
	value_list[2] = font;

	xcb_void_cookie_t gcCookie = xcb_create_gc_checked (connection,
			gc,
			window,
			mask,
			value_list );
	testCookie (gcCookie, connection, "can't create gc");

	fontCookie = xcb_close_font_checked (connection, font);
	testCookie (fontCookie, connection, "can't close font");

	return gc;
}

/*
*/
static void setCursor ( 
		xcb_connection_t *connection,
		xcb_screen_t     *screen,
		xcb_window_t      window,
		int               cursorId )
{
	xcb_font_t font = xcb_generate_id (connection);
	xcb_void_cookie_t fontCookie = xcb_open_font_checked (connection,
			font, strlen ("cursor"), "cursor" );
	
	testCookie (fontCookie, connection, "can't open font");

	xcb_cursor_t cursor = xcb_generate_id (connection);
	xcb_create_glyph_cursor (connection,
			cursor,
			font,
			font,
			cursorId,
			cursorId + 1,
			0, 0, 0, 0, 0, 0 );

	xcb_gcontext_t gc = xcb_generate_id (connection);

	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	uint32_t values_list[3];
	values_list[0] = screen->black_pixel;
	values_list[1] = screen->white_pixel;
	values_list[2] = font;

	xcb_void_cookie_t gcCookie = xcb_create_gc_checked (connection, gc, window, mask, values_list);
	testCookie (gcCookie, connection, "can't create gc");

	mask = XCB_CW_CURSOR;
	uint32_t value_list = cursor;
	xcb_change_window_attributes (connection, window, mask, &value_list);

	xcb_free_cursor (connection, cursor);

	fontCookie = xcb_close_font_checked (connection, font);
	testCookie (fontCookie, connection, "can't close font");
}

/*
*/
	int
main ()
{
	/* get the connection */
	int screenNum;
	xcb_connection_t *connection = xcb_connect (NULL, &screenNum);
	if (!connection) {
		fprintf (stderr, "ERROR: can't connect to an X server\n");
		return -1;
	}

	/* get the current screen */

	xcb_screen_iterator_t iter = xcb_setup_roots_iterator (xcb_get_setup (connection));

	/* we want the screen at index screenNum of the iterator */
	for (int i = 0; i < screenNum; ++i) {
		xcb_screen_next (&iter);
	}

	xcb_screen_t *screen = iter.data;

	if (!screen) {
		fprintf (stderr, "ERROR: can't get the current screen\n");
		xcb_disconnect (connection);
		return -1;
	}


	/* create the window */

	xcb_window_t window = xcb_generate_id (connection);
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2];
	values[0] = screen->white_pixel;
	values[1] = XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_POINTER_MOTION;

	xcb_void_cookie_t windowCookie = xcb_create_window_checked (connection,
			screen->root_depth,
			window,
			screen->root,
			20, 200, WIDTH, HEIGHT,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			mask, values );
	testCookie (windowCookie, connection, "can't create window");

	xcb_void_cookie_t mapCookie = xcb_map_window_checked (connection, window);
	testCookie (mapCookie, connection, "can't map window");

	setCursor (connection, screen, window, 68);

	xcb_flush(connection);

	/* event loop */

	uint8_t isHand = 0;

	while (1)
	{
		xcb_generic_event_t *event = xcb_poll_for_event (connection);
		if (event)
		{
			switch (event->response_type & ~0x80)
			{
				case XCB_EXPOSE:
				{
					char *text = "click here to change cursor";
					drawButton (connection,
							screen,
							window,
							(WIDTH - 7 * strlen(text)) / 2,
							(HEIGHT - 16) / 2,
							text );

					text = "Press ESC key to exit...";
					drawText (connection,
							screen,
							window,
							10,
							HEIGHT - 10,
							text );
					break;
				}
				case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;

					int length = strlen ("click here to change cursor");
					if ((press->event_x >= (WIDTH - 7 * length) / 2) &&
							(press->event_x <= ((WIDTH - 7 * length) / 2 + 7 * length + 6)) &&
							(press->event_y >= (HEIGHT - 16) / 2 - 19) &&
							(press->event_y <= ((HEIGHT - 16) / 2))) {
						isHand = 1 - isHand;
					}

					if (isHand) {
						setCursor (connection, screen, window, 58);
					}
					else {
						setCursor (connection, screen, window, 68);
					}
				}
				case XCB_KEY_RELEASE:
				{
					xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;

					switch (kr->detail)
					{
						/* ESC */
						case 9:
							free (event);
							xcb_disconnect (connection);
							return 0;
					}
				}
			}
			free (event);
		}
	}

	return 0;
}
