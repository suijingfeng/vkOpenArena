
// suijingfeng

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include "xcb_keysyms.h"


#include "win_public.h"
#include "sys_public.h"
#include "../client/client.h"


static qboolean isMouseActive = qfalse;

extern struct WinData_s s_xcb_win;


static xcb_key_symbols_t * keysyms_;

extern xcb_intern_atom_reply_t *atom_wm_delete_window;


static int win_center_x, win_center_y;



static void IN_ActivateMouse( void )
{
/*
      Grab the pointer.

xcb_grab_pointer_cookie_t xcb_grab_pointer(	
    xcb_connection_t *  c,
    uint8_t 	        owner_events,
    xcb_window_t 	    grab_window,
    uint16_t 	        event_mask,
    uint8_t 	        pointer_mode,
    uint8_t 	        keyboard_mode,
    xcb_window_t 	    confine_to,
    xcb_cursor_t 	    cursor,
    xcb_timestamp_t 	time )		


Parameters
c	The connection
owner_events	If 1, the grab_window will still get the pointer events.
                If 0, events are not reported to the grab_window.
                
grab_window	Specifies the window on which the pointer should be grabbed.
event_mask	Specifies which pointer events are reported to the client. 
TODO: which values?
pointer_mode	A bitmask of xcb_grab_mode_t values.
pointer_mode	
keyboard_mode	A bitmask of xcb_grab_mode_t values.
keyboard_mode	
confine_to	Specifies the window to confine the pointer in (the user will not be able to move the pointer out of that window). 
The special value XCB_NONE means don't confine the pointer.
cursor	Specifies the cursor that should be displayed or XCB_NONE to not change the cursor.
time	The time argument allows you to avoid certain circumstances that come up if applications take a long time to respond or if there are long network delays. Consider a situation where you have two applications, both of which normally grab the pointer when clicked on. If both applications specify the timestamp from the event, the second application may wake up faster and successfully grab the pointer before the first application. The first application then will get an indication that the other application grabbed the pointer before its request was processed. 
The special value XCB_CURRENT_TIME will be replaced with the current server time.
Returns
A cookie
Actively grabs control of the pointer. Further pointer events are reported only to the grabbing client. Overrides any active pointer grab by this client.

References XCB_GRAB_POINTER.
*/

	if ( isMouseActive )
	{
		return;
	}
	else
	{
		// Grabs the pointer actively
		// Pointer Grabbing  
		//
		// Xlib provides functions that you can use to control input from the pointer,
		// which usually is a mouse. Usually, as soon as keyboard and mouse events occur, 
		// the X server delivers them to the appropriate client, which is determined by
		// the window and input focus. The X server provides sufficient control over event
		// delivery to allow window managers to support mouse ahead and various other styles
		// of user interface. Many of these user interfaces depend upon synchronous delivery
		// of events. The delivery of pointer and keyboard events can be controlled independently.
		//
		// When mouse buttons or keyboard keys are grabbed, events will be sent to the grabbing
		// client rather than the normal client who would have received the event. If the keyboard
		// or pointer is in asynchronous mode, further mouse and keyboard events will continue to
		// be processed. If the keyboard or pointer is in synchronous mode, no further events
		// are processed until the grabbing client allows them (see XAllowEvents()). 
		//
		// The keyboard or pointer is considered frozen during this interval. The event that 
		// triggered the grab can also be replayed.
		//
		// Note that the logical state of a device (as seen by client applications) may lag
		// the physical state if device event processing is frozen.
		//
		// There are two kinds of grabs: active and passive. 
		// An active grab occurs when a single client grabs the keyboard and/or pointer explicitly
		// (see XGrabPointer() and XGrabKeyboard()). A passive grab occurs when clients grab a
		// particular keyboard key or pointer button in a window, and the grab will activate when
		// the key or button is actually pressed. Passive grabs are convenient for implementing 
		// reliable pop-up menus. For example, you can guarantee that the pop-up is mapped before
		// the up pointer button event occurs by grabbing a button requesting synchronous behavior.
		//
		// The down event will trigger the grab and freeze further processing of pointer events
		// until you have the chance to map the pop-up window. You can then allow further event
		// processing. The up event will then be correctly processed relative to the pop-up window.
		//
		// For many operations, there are functions that take a time argument. The X server
		// includes a timestamp in various events. One special time, called CurrentTime, 
		// represents the current server time. The X server maintains the time when 
		// the input focus was last changed, when the keyboard was last grabbed, when the
		// pointer was last grabbed, or when a selection was last changed. Your application
		// may be slow reacting to an event. 
		//
		// You often need some way to specify that yourb request should not occur 
		// if another application has in the meanwhile taken control of the keyboard,
		// pointer, or selection. By providing the timestamp from the event in the request, 
		// you can arrange that the operation not take effect if someone else has performed
		// an operation in the meanwhile.
		//
		// A timestamp is a time value, expressed in milliseconds. It typically is the time
		// since the last server reset. Timestamp values wrap around (after about 49.7 days).
		// The server, given its current time is represented by timestamp T, always interprets
		// timestamps from clients by treating half of the timestamp space as being later 
		// in time than T. One timestamp value, named CurrentTime, is never generated by the
		// server. This value is reserved for use in requests to represent the current server time.
		//
		xcb_warp_pointer( s_xcb_win.connection, XCB_NONE, s_xcb_win.hWnd,  
				0, 0, 0, 0, s_xcb_win.winWidth/2, s_xcb_win.winHeight/2);

		// xcb_flush (s_xcb_win.connection);

		uint32_t MASK =	XCB_EVENT_MASK_BUTTON_PRESS | 
				XCB_EVENT_MASK_BUTTON_RELEASE |
				XCB_EVENT_MASK_POINTER_MOTION;
		/*
		   XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS |
		   XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
		   XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEYMAP_STATE | XCB_EVENT_MASK_EXPOSURE |
		   XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE |
		   XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
		*/

		// move pointer to destination window area

		xcb_grab_pointer_cookie_t cookie_mouse = xcb_grab_pointer( 
				s_xcb_win.connection,
				1,			// get all pointer events specified by the following mask
				s_xcb_win.hWnd,		// grab the main game window
				MASK,			// which events to let through
				XCB_GRAB_MODE_ASYNC,	// pointer events should continue as normal
				XCB_GRAB_MODE_ASYNC,	// keyboard mode
				s_xcb_win.hWnd,		// confine_to = in which window should the cursor stay
				XCB_NONE,              // we change the cursor to whatever the user wanted
				XCB_CURRENT_TIME );
		xcb_flush (s_xcb_win.connection);

		
		xcb_grab_pointer_reply_t* pReply_mouse = 
			xcb_grab_pointer_reply(s_xcb_win.connection, cookie_mouse, NULL);
		
		if (pReply_mouse)
		{
			if (pReply_mouse->status == XCB_GRAB_STATUS_SUCCESS)
			{	
				Com_Printf( " Grab the pointer successfully! \n");

				/*
				   xcb_void_cookie_t xcb_change_pointer_control(
				   xcb_connection_t *  	c,
				   int16_t  	acceleration_numerator,
				   int16_t  	acceleration_denominator,
				   int16_t  	threshold,
				   uint8_t  	do_acceleration,
				   uint8_t  	do_threshold )
				   */
				// xcb_change_pointer_control(s_xcb_win.connection, 1, 4, 1, 1, 1);
			}
			else
			{
				Com_Printf(" Grab the pointer failed.\n");
			}
		}
		else
		{
			Com_Printf(" Grab the pointer failed.\n");
		}

		/*
		   xcb_grab_keyboard_cookie_t  cookie_keyboard = xcb_grab_keyboard( 
		   s_xcb_win.connection, 1, s_xcb_win.hWnd, 
		   XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);	

		   xcb_grab_keyboard_reply_t* pReply_keyboard = 
		   xcb_grab_keyboard_reply(s_xcb_win.connection, cookie_keyboard, NULL);	

		   if (pReply_keyboard )
		   {
		   if (pReply_keyboard->status == XCB_GRAB_STATUS_SUCCESS)
		   Com_Printf(" Grab the keyboard successfully. \n");
		   else
		   Com_Printf(" Grab the keyboard failed .\n");
		   }
		   free( pReply_keyboard );

*/

		free( pReply_mouse );




		isMouseActive = qtrue;
	}
	
	win_center_x = s_xcb_win.winWidth/2;
	win_center_y = s_xcb_win.winHeight/2;

}


static void IN_DeactivateMouse(void)
{

    // xcb_void_cookie_t xcb_ungrab_pointer( xcb_connection_t * c, xcb_timestamp_t time );	
    //  
    //  Parameters
    // c: The connection
    // time: Timestamp to avoid race conditions when running X over the network. 
    // The pointer will not be released if time is earlier than the 
    // last-pointer-grab time or later than the current X server time.
    // 
    // Releases the pointer and any queued events if you actively grabbed the pointer
    // before using xcb_grab_pointer, xcb_grab_button or within a normal button press.
    // EnterNotify and LeaveNotify events are generated.

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if ( isMouseActive == 0 )
        {
                //return if already deactive;
                return;
        }
	else
	{
		xcb_ungrab_pointer( s_xcb_win.connection, XCB_TIME_CURRENT_TIME);
		
		xcb_ungrab_keyboard( s_xcb_win.connection, XCB_TIME_CURRENT_TIME);	 	
		
		Com_Printf(" Ungrab the pointer.\n");		

		// xcb_void_cookie_t xcb_warp_pointer( xcb_connection_t *conn, 
		// xcb_window_t src_window, xcb_window_t dst_window, 
		// int16_t src_x, int16_t src_y, uint16_t src_width, uint16_t src_height, 
		// int16_t dst_x, int16_t dst_y);

		xcb_warp_pointer( s_xcb_win.connection, XCB_NONE, s_xcb_win.hWnd,
					0, 0, 0, 0, win_center_x, win_center_y); 
		xcb_flush (s_xcb_win.connection);
	/*
		xcb_grab_pointer_reply_t* pReply_mouse = 
			xcb_grab_pointer_reply(s_xcb_win.connection, cookie_mouse, NULL);
		
		if (pReply_mouse)
		{
			if (pReply_mouse->status == XCB_GRAB_STATUS_SUCCESS)
			{	
				Com_Printf( " Grab the pointer successfully! \n");

				xcb_change_pointer_control(s_xcb_win.connection, 1, 4, 1, 1, 1);
			}
			else
			{
				Com_Printf(" Grab the pointer failed.\n");
			}
		}
		else
		{
			Com_Printf(" Grab the pointer failed.\n");
		}
	*/
		isMouseActive = 0;
	}
}


static const char s_keytochar[ 128 ] =
{
//0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F 
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  '1',  '2',  '3',  '4',  '5',  '6',  // 0
 '7',  '8',  '9',  '0',  '-',  '=',  0x8,  0x9,  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 1
 'o',  'p',  '[',  ']',  0x0,  0x0,  'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 2
 '\'', 0x0,  0x0,  '\\', 'z',  'x',  'c',  'v',  'b',  'n',  'm',  ',',  '.',  '/',  0x0,  '*',  // 3

//0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F 
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  '!',  '@',  '#',  '$',  '%',  '^',  // 4
 '&',  '*',  '(',  ')',  '_',  '+',  0x8,  0x9,  'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 5
 'O',  'P',  '{',  '}',  0x0,  0x0,  'A',  'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 6
 '"',  0x0,  0x0,  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?',  0x0,  '*',  // 7
};



/*
typedef struct {
    
    // The type of the event, here it is xcb_button_press_event_t or xcb_button_release_event_t
    uint8_t         response_type; 
    xcb_button_t    detail;
    uint16_t        sequence;
    // Time, in milliseconds the event took place in
    xcb_timestamp_t time;          
    xcb_window_t    root;
    xcb_window_t    event;
    xcb_window_t    child;
    int16_t         root_x;
    int16_t         root_y;
    // The x coordinate where the mouse has been pressed in the window
    int16_t         event_x;       
    // The y coordinate where the mouse has been pressed in the window
    int16_t         event_y;       
    // A mask of the buttons (or keys) during the event
    uint16_t        state;         
    uint8_t         same_screen;
} xcb_button_press_event_t;

The state field is a mask of the buttons held down during the event.
It is a bitwise OR of any of the following (from the xcb_button_mask_t
and xcb_mod_mask_t enumerations):

XCB_BUTTON_MASK_1
XCB_BUTTON_MASK_2
XCB_BUTTON_MASK_3
XCB_BUTTON_MASK_4
XCB_BUTTON_MASK_5
XCB_MOD_MASK_SHIFT
XCB_MOD_MASK_LOCK
XCB_MOD_MASK_CONTROL
XCB_MOD_MASK_1
XCB_MOD_MASK_2
XCB_MOD_MASK_3
XCB_MOD_MASK_4
XCB_MOD_MASK_5

       Mouse movement events

Similar to mouse button press and release events, we also can be notified of
various mouse movement events. These can be split into two families. 
One is of mouse pointer movement while no buttons are pressed, and 
the second is a mouse pointer motion while one (or more) of the buttons are pressed
(this is sometimes called "a mouse drag operation", or just "dragging").
The following event masks may be added during the creation of our window:

XCB_EVENT_MASK_POINTER_MOTION: 
    events of the pointer moving in one of the windows controlled by our application,
    while no mouse button is held pressed.
XCB_EVENT_MASK_BUTTON_MOTION: 
    Events of the pointer moving while one or more of the mouse buttons is held pressed.
XCB_EVENT_MASK_BUTTON_1_MOTION:
    same as XCB_EVENT_MASK_BUTTON_MOTION, but only when the 1st mouse button is held pressed.
XCB_EVENT_MASK_BUTTON_2_MOTION, XCB_EVENT_MASK_BUTTON_3_MOTION,
XCB_EVENT_MASK_BUTTON_4_MOTION, XCB_EVENT_MASK_BUTTON_5_MOTION:
same as XCB_EVENT_MASK_BUTTON_1_MOTION, but respectively for 2nd, 3rd, 4th and 5th mouse button.
*/

// typedef xcb_button_press_event_t xcb_button_release_event_t;


static int XLateKey( xcb_key_press_event_t * ev, uint32_t *key )
{
  // static unsigned char buf[64];
  // static unsigned char bufnomod[2];
  // bufnomod[0] = '\0';

  *key = 0;
/*
  int shift = (ev->state & XCB_MOD_MASK_SHIFT);
  if(shift) {
      Com_Printf( "^2shift + ^7 %08X\n", key_value );
  }
*/ 
  
  int col;
  if (ev->state & ( XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_LOCK ))
      col = 1;  // shift || caps-lock
  else if (ev->state & XCB_BUTTON_MASK_1)
      col = 4;  // alt-gr
  else
      col = 0;
  
  // typedef uint32_t xcb_keysym_t;
  xcb_keysym_t keysym = xcb_key_press_lookup_keysym(keysyms_ , ev, col);

// The XLookupString function translates a key event to a KeySym and a string.
// The KeySym is obtained by using the standard interpretation of the Shift,
// Lock, group, and numlock modifiers as defined in the X Protocol specification.
// If the KeySym has been rebound (see XRebindKeysym), the bound string will be
// stored in the buffer. Otherwise, the KeySym is mapped, if possible, 
// to an ISO Latin-1 character or (if the Control modifier is on) to an ASCII control character, 
// and that character is stored in the buffer. 
//
// XLookupString returns the number of characters that are stored in the buffer.
  // XLookupRet = XLookupString(ev, (char*)buf, sizeof(buf), &keysym, 0);
 
// int XRebindKeysym(Display *display, KeySym keysym, KeySym list[], 
//          int mod_count, unsigned char *string, int num_bytes);
// 
// The XRebindKeysym function can be used to rebind the meaning of a KeySym for the client.
// It does not redefine any key in the X server but merely provides an easy way for long
// strings to be attached to keys. XLookupString returns this string when the appropriate
// set of modifier keys are pressed and when the KeySym would have been used for the translation.
// No text conversions are performed; the client is responsible for supplying appropriately
// encoded strings. Note that you can rebind a KeySym that may not exist.

  switch (keysym)
  {
  case XK_grave:
  case XK_twosuperior:
    *key = K_CONSOLE;
    return 0;

  case XK_KP_Page_Up:
  case XK_KP_9:  *key = K_KP_PGUP; break;
  case XK_Page_Up:   *key = K_PGUP; break;

  case XK_KP_Page_Down:
  case XK_KP_3: *key = K_KP_PGDN; break;
  case XK_Page_Down:   *key = K_PGDN; break;

  case XK_KP_Home: *key = K_KP_HOME; break;
  case XK_KP_7: *key = K_KP_HOME; break;
  case XK_Home:  *key = K_HOME; break;

  case XK_KP_End:
  case XK_KP_1:   *key = K_KP_END; break;
  case XK_End:   *key = K_END; break;

  case XK_KP_Left: *key = K_KP_LEFTARROW; break;
  case XK_KP_4: *key = K_KP_LEFTARROW; break;
  case XK_Left:  *key = K_LEFTARROW; break;

  case XK_KP_Right: *key = K_KP_RIGHTARROW; break;
  case XK_KP_6: *key = K_KP_RIGHTARROW; break;
  case XK_Right:  *key = K_RIGHTARROW;  break;

  case XK_KP_Down:
  case XK_KP_2:  *key = K_KP_DOWNARROW; break;

  case XK_Down:  *key = K_DOWNARROW; break;

  case XK_KP_Up:
  case XK_KP_8:  *key = K_KP_UPARROW; break;

  case XK_Up:    *key = K_UPARROW;   break;

  case XK_Escape: *key = K_ESCAPE;    break;

  case XK_KP_Enter: *key = K_KP_ENTER;  break;
  case XK_Return: *key = K_ENTER;    break;

  case XK_Tab:    *key = K_TAB;      break;

  case XK_F1:    *key = K_F1;       break;

  case XK_F2:    *key = K_F2;       break;

  case XK_F3:    *key = K_F3;       break;

  case XK_F4:    *key = K_F4;       break;

  case XK_F5:    *key = K_F5;       break;

  case XK_F6:    *key = K_F6;       break;

  case XK_F7:    *key = K_F7;       break;

  case XK_F8:    *key = K_F8;       break;

  case XK_F9:    *key = K_F9;       break;

  case XK_F10:    *key = K_F10;      break;

  case XK_F11:    *key = K_F11;      break;

  case XK_F12:    *key = K_F12;      break;

    // bk001206 - from Ryan's Fakk2
    //case XK_BackSpace: *key = 8; break; // ctrl-h
  case XK_BackSpace: *key = K_BACKSPACE; break; // ctrl-h

  case XK_KP_Delete:
  case XK_KP_Decimal: *key = K_KP_DEL; break;
  case XK_Delete: *key = K_DEL; break;

  case XK_Pause:  *key = K_PAUSE;    break;

  case XK_Shift_L:
  case XK_Shift_R:  *key = K_SHIFT;   break;

  case XK_Execute:
  case XK_Control_L:
  case XK_Control_R:  *key = K_CTRL;  break;

  case XK_Alt_L:
  case XK_Meta_L:
  case XK_Alt_R:
  case XK_Meta_R: *key = K_ALT;     break;

  case XK_KP_Begin: *key = K_KP_5;  break;

  case XK_Insert:   *key = K_INS; break;
  case XK_KP_Insert:
  case XK_KP_0: *key = K_KP_INS; break;

  case XK_KP_Multiply: *key = '*'; break;
  case XK_KP_Add:  *key = K_KP_PLUS; break;
  case XK_KP_Subtract: *key = K_KP_MINUS; break;
  case XK_KP_Divide: *key = K_KP_SLASH; break;

  case XK_exclam: *key = '1'; break;
  case XK_at: *key = '2'; break;
  case XK_numbersign: *key = '3'; break;
  case XK_dollar: *key = '4'; break;
  case XK_percent: *key = '5'; break;
  case XK_asciicircum: *key = '6'; break;
  case XK_ampersand: *key = '7'; break;
  case XK_asterisk: *key = '8'; break;
  case XK_parenleft: *key = '9'; break;
  case XK_parenright: *key = '0'; break;

  // weird french keyboards ..
  // NOTE: console toggle is hardcoded in cl_keys.c, can't be unbound
  //   cleaner would be .. using hardware key codes instead of the key syms
  //   could also add a new K_KP_CONSOLE
  //case XK_twosuperior: *key = '~'; break;

  case XK_space:
  case XK_KP_Space: *key = K_SPACE; break;

  case XK_Menu:	*key = K_MENU; break;
  case XK_Print: *key = K_PRINT; break;
  case XK_Super_L:
  case XK_Super_R: *key = K_SUPER; break;
  case XK_Num_Lock: *key = K_KP_NUMLOCK; break;
  case XK_Caps_Lock: *key = K_CAPSLOCK; break;
  case XK_Scroll_Lock: *key = K_SCROLLOCK; break;
  case XK_backslash: *key = '\\'; break;

  default:
    break;

  }

  return 0;
}


static void Sys_SendKeyEvents( void )
{
	/*
	   uint8_t 	response_type
	   uint8_t 	pad0
	   uint16_t 	sequence
	   uint32_t 	pad[7]
	   uint32_t 	full_sequence
	*/
	xcb_generic_event_t *e;

	while( (e = xcb_poll_for_event(s_xcb_win.connection)) )
	{
		switch( e->response_type & ~0x80 )
		{
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;
				//int t = Sys_XTimeToSysTime( ev->time );

				if ( ev->detail == 0x31 )
				{
					// open pull down console
					Com_QueueEvent( Sys_Milliseconds( ), SE_KEY, K_CONSOLE, qtrue, 0, NULL );
				}
				else if ( ev->detail == 0x09 )
				{
					// menu
					Com_QueueEvent( Sys_Milliseconds( ), SE_KEY, K_ESCAPE, qtrue, 0, NULL );
				}
				else
				{
					uint32_t key_value = 0;

					XLateKey( ev, &key_value );

					if( key_value == K_BACKSPACE ) {
						Com_QueueEvent(Sys_Milliseconds( ), SE_CHAR, ('h'-'a'+1), 0, 0, NULL );
					}

					// !directMap() && 
					if ( (ev->detail < 0x3F) )
					{

						char ch = s_keytochar[ ev->detail ];

						if ( ch >= 'a' && ch <= 'z' )
						{
							int isShift = ev->state & XCB_MOD_MASK_SHIFT;
							int isCapLock = ev->state & XCB_MOD_MASK_LOCK;

							if ( isShift ^ isCapLock )
							{
								ch = ch - 'a' + 'A';
							}                            
						}
						else
						{
							ch = s_keytochar[ ev->detail | ((!!(ev->state & XCB_MOD_MASK_SHIFT))<<6) ];
						}

						Com_QueueEvent( Sys_Milliseconds( ), SE_KEY, ch, qtrue, 0, NULL );

						Com_QueueEvent( Sys_Milliseconds( ), SE_CHAR, ch, 0, 0, NULL );

					}



					/*
					   else if( keys[K_CTRL].down && key_value >= 'a' && key_value <= 'z' )
					   {
					   Com_QueueEvent(Sys_Milliseconds( ), SE_CHAR, (key_value-'a'+1), 0, 0, NULL );
					   }
					   */
				}

			} break;

            case XCB_KEY_RELEASE:
            {
                xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;
 
                uint32_t key_value = 0;
                //key = ev->detail;
			    
                XLateKey( ev, &key_value );
			    
               
                if (key_value >= 'A' && key_value <= 'Z')
                    key_value = key_value - 'A' + 'a';

                // Com_QueueEvent( 0, SE_KEY, key_value, qfalse, 0, NULL );
                if ( (ev->detail < 0x3F) )
                {
 
					char ch = s_keytochar[ ev->detail ];

					if ( ch >= 'a' && ch <= 'z' )
					{
						int isShift = ev->state & XCB_MOD_MASK_SHIFT;
						int isCapLock = ev->state & XCB_MOD_MASK_LOCK;

						if ( isShift ^ isCapLock )
						{
							ch = ch - 'a' + 'A';
						}                            
					}
					else
					{
						ch = s_keytochar[ ev->detail | ((!!(ev->state & XCB_MOD_MASK_SHIFT))<<6) ];
					}

					Com_QueueEvent( Sys_Milliseconds( ), SE_KEY, ch, qfalse, 0, NULL );

					// Com_QueueEvent( t, SE_CHAR, ch, 0, 0, NULL );
                }
            } break;

            case XCB_EXPOSE:
            {
                Com_Printf ("XCB_EXPOSE:\n");

            } break;
   
            case XCB_CREATE_NOTIFY:
            {
                xcb_create_notify_event_t * ev = (xcb_create_notify_event_t *)e;
                
                printf ("CREATE_NOTIFY, window: %d (%d, %d, %d, %d) \n",
                        ev->window, ev->x, ev->y, ev->width, ev->height );

/*
uint8_t 	response_type 
uint8_t 	pad0
uint16_t 	sequence
xcb_window_t 	parent
xcb_window_t 	window
int16_t 	x
int16_t 	y
uint16_t 	width
uint16_t 	height
uint16_t 	border_width
uint8_t 	override_redirect
uint8_t 	pad1
*/
            } break;

            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t* ev = (xcb_button_press_event_t *)e;

                switch (ev->detail)
                {
                    case XCB_BUTTON_INDEX_1:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MOUSE1, qtrue, 0, NULL );
                        // printf ("Button left pressed \n");
                    } break; //  the left mouse button

                    case XCB_BUTTON_INDEX_3:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MOUSE2, qtrue, 0, NULL );
                        // printf ("Button right pressed \n");
                    } break; // the right mouse button

                    case XCB_BUTTON_INDEX_2:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MOUSE3, qtrue, 0, NULL );
                        // printf ("Button middle pressed \n");
                    } break; //  the right mouse button

                    case XCB_BUTTON_INDEX_4:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
                        // Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
                        Com_Printf(" mouse wheel scroll up press. \n");
                    } break; //  mouse wheel scroll up
                    
                    case XCB_BUTTON_INDEX_5:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
                        // Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
                        Com_Printf(" mouse wheel scroll down press. \n");
                    } break;//  mouse scroll down

                    case 8:
                    {
                        printf ("mouse wheel scroll down. \n");
                        Com_QueueEvent( Sys_Milliseconds( ), SE_KEY, 'f', qtrue, 0, NULL );
                    } break;

                    case 9:
                    {
                        printf ("mouse wheel scroll down. \n");

                    } break;

                    default:
                        printf ("Button %d pressed in window %d, at coordinates (%d,%d)\n",
                                ev->detail, ev->event, ev->event_x, ev->event_y);
                }
            } break;

            case XCB_BUTTON_RELEASE:
			{
				xcb_button_release_event_t* ev = (xcb_button_release_event_t *)e;

				switch (ev->detail)
                {
                    case XCB_BUTTON_INDEX_1:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MOUSE1, qfalse, 0, NULL );
                        // printf ("Button left pressed \n");
                    } break; //  the left mouse button

                    case XCB_BUTTON_INDEX_3:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MOUSE2, qfalse, 0, NULL );
                        // printf ("Button right pressed \n");
                    } break; // the right mouse button

                    case XCB_BUTTON_INDEX_2:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MOUSE3, qfalse, 0, NULL );
                        // printf ("Button middle pressed \n");
                    } break; //  the right mouse button

                    case XCB_BUTTON_INDEX_4:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
                        Com_Printf(" mouse wheel scroll up release. \n");
                    } break; //  mouse wheel scroll up
                    
                    case XCB_BUTTON_INDEX_5:
                    {
                        Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
                        // Com_QueueEvent(Sys_Milliseconds( ), SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
                        Com_Printf(" mouse wheel scroll down release. \n");
                    } break;//  mouse scroll down

                    case 8:
                    {
                        printf ("mouse wheel scroll down. \n");
                        Com_QueueEvent( Sys_Milliseconds( ), SE_KEY, 'f', qfalse, 0, NULL );
                    } break;

                    case 9:
                    {
                        printf ("mouse wheel scroll down. \n");

                    } break;

                    default:
                        printf ("Button %d pressed in window %d, at coordinates (%d,%d)\n",
                                ev->detail, ev->event, ev->event_x, ev->event_y);
                }

			}break;
		
	case XCB_MOTION_NOTIFY:
	{
		if ( isMouseActive )
		{
			//  Mouse movement events
			//  
			//  Similar to mouse button press and release events, 
			//  we also can be notified of various mouse movement events. 
			//  These can be split into two families. 
			//  One is of mouse pointer movement while no buttons are pressed, 
			//  and the second is a mouse pointer motion while one (or more) 
			//  of the buttons are pressed (this is sometimes called "a mouse
			//  drag operation", or just "dragging"). 
			//
			//  The following event masks may be added during the creation of
			//  our window to register for these events:
			//
			//  XCB_EVENT_MASK_POINTER_MOTION   // motion with no mouse button held
			//  XCB_EVENT_MASK_BUTTON_MOTION    // motion with one or more mouse buttons held
			//  XCB_EVENT_MASK_BUTTON_1_MOTION  // motion while only 1st mouse button is held
			//  XCB_EVENT_MASK_BUTTON_2_MOTION  // and so on...
			//  XCB_EVENT_MASK_BUTTON_3_MOTION
			//  XCB_EVENT_MASK_BUTTON_4_MOTION
			//  XCB_EVENT_MASK_BUTTON_5_MOTION
			//int t = Sys_Milliseconds();
			xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)e;

			/*
			   uint8_t 	response_type
			   uint8_t 	detail
			   uint16_t 	sequence
			   xcb_timestamp_t 	time
			   xcb_window_t 	root
			   xcb_window_t 	event
			   xcb_window_t 	child
			   int16_t 	root_x
			   int16_t 	root_y
			   int16_t 	event_x
			   int16_t 	event_y
			   uint16_t 	state
			   uint8_t 	same_screen
			   uint8_t 	pad0
			   */
			// root_x, root_y: coordinate in destop coordinate system
			// event_x, event_y: coordinate in renderer arena .

			int dx = ev->event_x - win_center_x;
			int dy = ev->event_y - win_center_y;

			// accurate the delta
			if ( (dx != 0) || (dy != 0 ) )
			{
				Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );

				xcb_warp_pointer( s_xcb_win.connection, XCB_NONE, s_xcb_win.hWnd,
						0, 0, 0, 0, win_center_x, win_center_y);

				// Com_Printf ("MOUSE_MOTION: (%d, %d, %d, %d), win center,(%d, %d)\n",
				//	ev->root_x, ev->root_y, ev->event_x, ev->event_y, win_center_x, win_center_y);
			}
		}
	} break;

            case XCB_CLIENT_MESSAGE:
            {
                if ((*(xcb_client_message_event_t *)e).data.data32[0] ==
                        (*atom_wm_delete_window).atom)
                {
                	 // Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
			 Cmd_Clear();
                         Com_Quit_f();
                }
            } break;

            case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t * ev = (xcb_configure_notify_event_t *) e;
/*
                xcb_configure_notify_event_t
                {
                    uint8_t 	response_type
                        uint8_t 	pad0
                        uint16_t 	sequence
                        xcb_window_t 	event
                        xcb_window_t 	window
                        xcb_window_t 	above_sibling
                        int16_t 	x
                        int16_t 	y
                        uint16_t 	width
                        uint16_t 	height
                        uint16_t 	border_width
                        uint8_t 	override_redirect
                        uint8_t 	pad1
                }
*/
                Com_Printf ("CONFIGURE_NOTIFY: (%d, %d, %d, %d)\n", ev->x, ev->y, ev->width, ev->height);

		if( !WinSys_IsWinFullscreen() && !WinSys_IsWinMinimized() )
		{

		}                
 
		Key_ClearStates();
                win_center_x =  ev->width / 2;
                win_center_y =  ev->height / 2;
            } break;

            case XCB_REPARENT_NOTIFY:
            {
                xcb_reparent_notify_event_t * ev = (xcb_reparent_notify_event_t *) e;
                printf ("REPARENT_NOTIFY: (window: %d, parent: %d)\n", ev->window, ev->parent);

/*
uint8_t 	response_type 
uint8_t 	pad0
uint16_t 	sequence
xcb_window_t 	event
xcb_window_t 	window
xcb_window_t 	parent
int16_t 	x
int16_t 	y
uint8_t 	override_redirect
uint8_t 	pad1 [3]
 */

            }break;

            case XCB_ENTER_NOTIFY:
            {
                xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)e;

                printf ("Mouse entered window %d, at coordinates (%d,%d)\n",
                         ev->event, ev->event_x, ev->event_y);
                break;
            }

            case XCB_LEAVE_NOTIFY:
            {
                xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)e;

                printf ("Mouse left window %d, at coordinates (%d,%d)\n",
                        ev->event, ev->event_x, ev->event_y);
                break;
            }


            case XCB_FOCUS_IN:
            {
                Key_ClearStates();
                xcb_focus_in_event_t * ev = (xcb_focus_in_event_t *) e;
                Com_Printf ("FOCUS_IN: detal: %d, event:%d, mode:%d, sequence: %d. \n",
                        ev->detail, ev->event, ev->mode, ev->sequence);

            } break;

            case XCB_FOCUS_OUT:
            {
                Key_ClearStates();

                xcb_focus_out_event_t * ev = (xcb_focus_out_event_t *) e;
                Com_Printf ("FOCUS_OUT: detal: %d, event:%d, mode:%d, sequence: %d. \n",
                        ev->detail, ev->event, ev->mode, ev->sequence);
            } break;

            case XCB_MAP_NOTIFY:
            {
                xcb_map_notify_event_t * ev = (xcb_map_notify_event_t *)e;

                /*
                uint8_t 	response_type
                uint8_t 	pad0
                uint16_t 	sequence
                xcb_window_t 	event
                xcb_window_t 	window
                uint8_t 	override_redirect
                uint8_t 	pad1[3]
                */
                printf (" MAP_NOTIFY, response_type:%d, sequence:%d, event:%d, window:%d, override_redirect:%d\n",
                        ev->response_type, ev->sequence, ev->event, ev->window, ev->override_redirect);
            } break;

            case XCB_UNMAP_NOTIFY:
            {
                xcb_unmap_notify_event_t * ev = (xcb_unmap_notify_event_t *)e;
                printf (" UNMAP_NOTIFY, response_type:%d, sequence:%d, event:%d, window:%d, from_configure:%d\n",
                        ev->response_type, ev->sequence, ev->event, ev->window, ev->from_configure);
            } break;

            case XCB_VISIBILITY_NOTIFY:
            {
                
                xcb_visibility_notify_event_t * ev = (xcb_visibility_notify_event_t *) e;
                
                printf (" VISIBILITY_NOTIFY, response_type:%d, sequence:%d, window:%d, state:%d\n",
                        ev->response_type, ev->sequence, ev->window, ev->state);

            } break;


            case XCB_DESTROY_NOTIFY:
            {
                xcb_destroy_notify_event_t * ev = (xcb_destroy_notify_event_t *) e;
                
                printf (" DESTROY_NOTIFY, response_type:%d, sequence:%d, event:%d, window:%d. \n",
                        ev->response_type, ev->sequence, ev->event, ev->window);
            } break;

            case XCB_KEYMAP_NOTIFY:
            {
                /*
                xcb_keymap_notify_event_t * ev = (xcb_keymap_notify_event_t * ) e;
                printf (" KEYMAP_NOTIFY, response_type:%d \n",
                        ev->response_type);

                int i = 0;
                for(i = 0; i < 32; ++i)
                {
                    printf("%d, ", ev->keys[i]);
                }

                printf("\n");
                */
            } break;

            case XCB_PROPERTY_NOTIFY:
            {
            
                xcb_property_notify_event_t * ev = (xcb_property_notify_event_t *) e ;

                printf (" PROPERTY_NOTIFY, response_type:%d, sequence:%d, state:%d, window:%d. \n",
                        ev->response_type, ev->sequence, ev->state, ev->window);

/*                
                uint8_t 	response_type
                uint16_t 	sequence
                xcb_window_t 	window
                xcb_atom_t 	atom
                xcb_timestamp_t 	time
                uint8_t 	state
*/
                

            } break;

            default: {
                /* Unknown event type, ignore it */
                printf("Unknown event: %d\n", e->response_type);
            }break;


        }

        /* Free the Generic Event */
        free (e);
    }
    

}


void IN_Frame( void )
{
	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading

	if( Key_GetCatcher( ) & KEYCATCH_CONSOLE)
	{
		// Console is down in windowed mode
                 if( WinSys_IsWinFullscreen() == 0)
                        IN_DeactivateMouse( );
	}
	else if( clc.state != CA_DISCONNECTED && clc.state != CA_ACTIVE )
	{
		// Loading in windowed mode
		if( WinSys_IsWinFullscreen() == 0 )
			IN_DeactivateMouse( );
	}
	else if( WinSys_IsWinMinimized() || WinSys_IsWinLostFocused() )
	{
		IN_DeactivateMouse( );
	}
	else{
		IN_ActivateMouse( );
	}

	Sys_SendKeyEvents( );
}


void Sys_HandleUserInputEvents(void)
{
    ;
}


void IN_Init( void )
{
	Com_Printf(" ...Input Initialization ( with xcb) ...\n");

	if( s_xcb_win.connection == NULL )
	{
		Com_Printf( "WARNNING: IN_Init called before xcb_create_window. \n" );
		return;
	}

	Cmd_AddCommand("in_restart", IN_Restart);

	keysyms_ = xcb_key_symbols_alloc(s_xcb_win.connection);
}


void IN_Shutdown(void)
{
	isMouseActive = qfalse;

	Cmd_RemoveCommand("in_restart");

    	xcb_key_symbols_free(keysyms_);
}


void IN_Restart( void )
{
	IN_Shutdown();
	IN_Init(); 
}

/*

 The keyboard focus

There may be many windows on a screen, but only a single keyboard attached to them.
How does the X server then know which window should be sent a given keyboard input?
This is done using the keyboard focus. Only a single window on the screen may have
the keyboard focus at a given time. There is a XCB function that allows a program
to set the keyboard focus to a given window. The user can usually set the keyboard
focus using the window manager (often by clicking on the title bar of the desired
window). Once our window has the keyboard focus, every key press or key release will
cause an event to be sent to our program (if it registered for these event types...).

*/
