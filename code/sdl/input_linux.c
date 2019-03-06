#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "input.h"
#include "local.h"
#include "../client/client.h"

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask | FocusChangeMask )

cvar_t *in_subframe;
cvar_t *in_dgamouse; // user pref for dga mouse
cvar_t *in_forceCharset;
static cvar_t *in_nograb; // this is strictly for developers


extern Display *dpy;
extern Window win;
extern int window_width;
extern int window_height;
extern Atom wmDeleteEvent;
extern glwstate_t glw_state;
extern qboolean window_created;

//#define KBD_DBG
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

static qboolean gw_minimized = qfalse; // this will be always true for dedicated servers

// Time mouse was reset, we ignore the first 50ms of the mouse to allow settling of events
static int mouseResetTime = 0;
#define MOUSE_RESET_DELAY 50

static cvar_t *in_mouse;

static cvar_t *in_shiftedKeys; // obey modifiers for certain keys in non-console (comma, numbers, etc)



static qboolean mouse_avail;
static qboolean mouse_active = qfalse;

static int mouse_accel_numerator;
static int mouse_accel_denominator;
static int mouse_threshold;

static int win_x, win_y;
static qboolean window_focused = qfalse;
static int mwx, mwy;
static int mx = 0, my = 0;


static char *XLateKey( XKeyEvent *ev, int *key )
{
  static unsigned char buf[64];
  static unsigned char bufnomod[2];
  KeySym keysym;
  int XLookupRet;

  *key = 0;

  XLookupRet = XLookupString(ev, (char*)buf, sizeof(buf), &keysym, 0);
#ifdef KBD_DBG
  Com_Printf( "XLookupString ret: %d buf: %s keysym: %x\n", XLookupRet, buf, (int)keysym) ;
#endif

  if (!in_shiftedKeys->integer) {
    // also get a buffer without modifiers held
    ev->state = 0;
    XLookupRet = XLookupString(ev, (char*)bufnomod, sizeof(bufnomod), &keysym, 0);
#ifdef KBD_DBG
    Com_Printf( "XLookupString (minus modifiers) ret: %d buf: %s keysym: %x\n", XLookupRet, buf, (int)keysym );
#endif
  } else {
    bufnomod[0] = '\0';
  }

  switch (keysym)
  {
  case XK_grave:
  case XK_twosuperior:
    *key = K_CONSOLE;
    buf[0] = '\0';
    return (char*)buf;

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
  case XK_Right:  *key = K_RIGHTARROW;    break;

  case XK_KP_Down:
  case XK_KP_2:  if ( Key_GetCatcher() && (buf[0] || bufnomod[0]) )
                   *key = 0;
                 else
                   *key = K_KP_DOWNARROW;
                 break;

  case XK_Down:  *key = K_DOWNARROW; break;

  case XK_KP_Up:
  case XK_KP_8:  if ( Key_GetCatcher() && (buf[0] || bufnomod[0]) )
                   *key = 0;
                 else
                   *key = K_KP_UPARROW;
                 break;

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
    //Com_Printf( "unknown keysym: %08X\n", keysym );
    if (XLookupRet == 0)
    {
      if (com_developer->value)
      {
        Com_Printf( "Warning: XLookupString failed on KeySym %d\n", (int)keysym );
      }
      buf[0] = '\0';
      return (char*)buf;
    }
    else
    {
      // XK_* tests failed, but XLookupString got a buffer, so let's try it
      if (in_shiftedKeys->integer) {
        *key = *(unsigned char *)buf;
        if (*key >= 'A' && *key <= 'Z')
          *key = *key - 'A' + 'a';
        // if ctrl is pressed, the keys are not between 'A' and 'Z', for instance ctrl-z == 26 ^Z ^C etc.
        // see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=19
        else if (*key >= 1 && *key <= 26)
          *key = *key + 'a' - 1;
      } else {
        *key = bufnomod[0];
      }
    }
    break;
  }

  return (char*)buf;
}


// ========================================================================
// makes a null cursor
// ========================================================================

static Cursor CreateNullCursor( Display *display, Window root )
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Cursor cursor;

	cursormask = XCreatePixmap( display, root, 1, 1, 1/*depth*/ );
	xgc.function = GXclear;
	gc = XCreateGC( display, cursormask, GCFunction, &xgc );
	XFillRectangle( display, cursormask, gc, 0, 0, 1, 1 );
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	cursor = XCreatePixmapCursor( display, cursormask, cursormask, &dummycolour, &dummycolour, 0, 0 );
	XFreePixmap( display, cursormask );
	XFreeGC( display, gc );
	return cursor;
}


static void install_mouse_grab( void )
{
	int res;

	// move pointer to destination window area
	XWarpPointer( dpy, None, win, 0, 0, 0, 0, window_width / 2, window_height / 2 );

	XSync( dpy, False );

	// hide cursor
	XDefineCursor( dpy, win, CreateNullCursor( dpy, win ) );

	// save old mouse settings
	XGetPointerControl( dpy, &mouse_accel_numerator, &mouse_accel_denominator, &mouse_threshold );

	// do this earlier?
	res = XGrabPointer( dpy, win, False, MOUSE_MASK, GrabModeAsync, GrabModeAsync, win, None, CurrentTime );
	if ( res != GrabSuccess )
	{
		//Com_Printf( S_COLOR_YELLOW "Warning: XGrabPointer() failed\n" );
	}
	else
	{
		// set new mouse settings
		XChangePointerControl( dpy, True, True, 1, 1, 1 );
	}

	XSync( dpy, False );

	mouseResetTime = Sys_Milliseconds();

#ifdef HAVE_XF86DGA
	if ( in_dgamouse->integer )
	{
		if ( !glw_state.dga_ext )
		{
			Cvar_Set( "in_dgamouse", "0" );
		}
		else
		{
			DGA_Mouse( qtrue );
			XWarpPointer( dpy, None, win, 0, 0, 0, 0, window_width / 2, window_height / 2 );
		}
	}
	else
#endif /* HAVE_XF86DGA */
	{
		mwx = window_width / 2;
		mwy = window_height / 2;
		mx = my = 0;
	}

	XSync( dpy, False );
}


static void install_kb_grab( void )
{
	int res;

	res = XGrabKeyboard( dpy, win, False, GrabModeAsync, GrabModeAsync, CurrentTime );
	if ( res != GrabSuccess )
	{
		//Com_Printf( S_COLOR_YELLOW "Warning: XGrabKeyboard() failed\n" );
	}

	XSync( dpy, False );
}


static void uninstall_mouse_grab( void )
{
#ifdef HAVE_XF86DGA
	if ( in_dgamouse->integer )
	{
		if ( com_developer->integer )
		{
			Com_Printf( "DGA Mouse - Disabling DGA DirectVideo\n" );
		}
		DGA_Mouse( qfalse );
	}
#endif /* HAVE_XF86DGA */

	// restore mouse settings
	XChangePointerControl( dpy, qtrue, qtrue, mouse_accel_numerator, 
		mouse_accel_denominator, mouse_threshold );

	XWarpPointer( dpy, None, win, 0, 0, 0, 0, window_width / 2, window_height / 2 );

	XUngrabPointer( dpy, CurrentTime );
	XUngrabKeyboard( dpy, CurrentTime );

	// show cursor
	XUndefineCursor( dpy, win );

	XSync( dpy, False );
}


static void uninstall_kb_grab( void )
{
	XUngrabKeyboard( dpy, CurrentTime );

	XSync( dpy, False );
}


// bk001206 - from Ryan's Fakk2
/**
 * XPending() actually performs a blocking read 
 *  if no events available. From Fakk2, by way of
 *  Heretic2, by way of SDL, original idea GGI project.
 * The benefit of this approach over the quite
 *  badly behaved XAutoRepeatOn/Off is that you get
 *  focus handling for free, which is a major win
 *  with debug and windowed mode. It rests on the
 *  assumption that the X server will use the
 *  same timestamp on press/release event pairs 
 *  for key repeats.
 */
static qboolean X11_PendingInput( void )
{
	assert(dpy != NULL);

	// Flush the display connection and look to see if events are queued
	XFlush( dpy );

	if ( XEventsQueued( dpy, QueuedAlready ) )
	{
		return qtrue;
	}

	// More drastic measures are required -- see if X is ready to talk
	{
		static struct timeval zero_time;
		int x11_fd;
		fd_set fdset;

		x11_fd = ConnectionNumber( dpy );
		FD_ZERO( &fdset );
		FD_SET( x11_fd, &fdset );
		if ( select( x11_fd+1, &fdset, NULL, NULL, &zero_time ) == 1 )
		{
			return( XPending( dpy ) );
		}
	}

	// Oh well, nothing is ready ..
	return qfalse;
}


static qboolean repeated_press( XEvent *event )
{
	XEvent peek;

	assert( dpy != NULL );

	if ( X11_PendingInput() )
	{
		XPeekEvent( dpy, &peek );

		if ( ( peek.type == KeyPress ) &&
			 ( peek.xkey.keycode == event->xkey.keycode ) &&
			 ( peek.xkey.time == event->xkey.time ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}


static qboolean WindowMinimized( Display *dpy, Window win )
{
	unsigned long i, num_items, bytes_after;
	Atom actual_type, *atoms, nws, nwsh;
	int actual_format;

	nws = XInternAtom( dpy, "_NET_WM_STATE", True );
	if ( nws == BadValue || nws == None )
		return qfalse;

	nwsh = XInternAtom( dpy, "_NET_WM_STATE_HIDDEN", True );
	if ( nwsh == BadValue || nwsh == None )
		return qfalse;

	atoms = NULL;

	XGetWindowProperty( dpy, win, nws, 0, 0x7FFFFFFF, False, XA_ATOM,
		&actual_type, &actual_format, &num_items,
		&bytes_after, (unsigned char**)&atoms );

	for ( i = 0; i < num_items; i++ )
	{
		if ( atoms[i] == nwsh )
		{
			XFree( atoms );
			return qtrue;
		}
	}

	XFree( atoms );
	return qfalse;
}


static qboolean directMap( const byte chr )
{
	if ( !in_forceCharset->integer )
		return qtrue;

	switch ( chr ) // edit control sequences
	{
		case 'c'-'a'+1:
		case 'v'-'a'+1:
		case 'h'-'a'+1:
		case 'a'-'a'+1:
		case 'e'-'a'+1:
		case 0xC: // CTRL+L
			return qtrue;
	}
	if ( chr < ' ' || chr > 127 || in_forceCharset->integer > 1 )
		return qfalse;
	else
		return qtrue;
}

/*
============
Cvar_SetIntegerValue
============
*/
void Cvar_SetIntegerValue( const char *var_name, int value ) {
	char	val[32];

	sprintf( val, "%i", value );
	Cvar_Set( var_name, val );
}




void Sys_SendKeyEvents( void )
{
	XEvent event;
	int b;
	int key;
	qboolean dowarp = qfalse;
	char *p;
	int dx, dy;
	int t = 0; // default to 0 in case we don't set
	qboolean btn_press;
	char buf[2];

	if ( !dpy )
		return;

	while( XPending( dpy ) )
	{
		XNextEvent( dpy, &event );

		switch( event.type )
		{

		case ClientMessage:

			if ( event.xclient.data.l[0] == wmDeleteEvent ) {
				Cmd_Clear();
				Com_Quit_f();
			}
			break;

		case KeyPress:
			// Com_Printf("^2K+^7 %08X\n", event.xkey.keycode );
			//t = Sys_XTimeToSysTime( event.xkey.time );
            t = Sys_Milliseconds();
			if ( event.xkey.keycode == 0x31 )
			{
				key = K_CONSOLE;
				p = "";
			}
			else
			{
				int shift = (event.xkey.state & 1);
				p = XLateKey( &event.xkey, &key );
				if ( *p && event.xkey.keycode == 0x5B )
				{
					p = ".";
				}
				else
				if ( !directMap( *p ) && event.xkey.keycode < 0x3F )
				{
					char ch;
					ch = s_keytochar[ event.xkey.keycode ];
					if ( ch >= 'a' && ch <= 'z' )
					{
						unsigned int capital;
						XkbGetIndicatorState( dpy, XkbUseCoreKbd, &capital );
						capital &= 1;
						if ( capital ^ shift )
						{
							ch = ch - 'a' + 'A';
						}
					}
					else
					{
						ch = s_keytochar[ event.xkey.keycode | (shift<<6) ];
					}
					buf[0] = ch;
					buf[1] = '\0';
					p = buf;
				}
			}
			if (key)
			{
				Com_QueueEvent( t, SE_KEY, key, qtrue, 0, NULL );
			}
			while (*p)
			{
				Com_QueueEvent( t, SE_CHAR, *p++, 0, 0, NULL );
			}
			break; // case KeyPress

		case KeyRelease:

			if ( repeated_press( &event ) )
				break; // XNextEvent( dpy, &event )

			//t = Sys_XTimeToSysTime( event.xkey.time );
            t = Sys_Milliseconds();
#if 0
			Com_Printf("^5K-^7 %08X %s\n",
				event.xkey.keycode,
				X11_PendingInput()?"pending":"");
#endif
			XLateKey( &event.xkey, &key );
			Com_QueueEvent( t, SE_KEY, key, qfalse, 0, NULL );

			break; // case KeyRelease

		case MotionNotify:
			if ( IN_MouseActive() )
			{
				//t = Sys_XTimeToSysTime( event.xkey.time );
                t = Sys_Milliseconds();
#ifdef HAVE_XF86DGA
				if ( in_dgamouse->integer )
				{
					mx += event.xmotion.x_root;
					my += event.xmotion.y_root;
					if (t - mouseResetTime > MOUSE_RESET_DELAY )
					{
						Com_QueueEvent( t, SE_MOUSE, mx, my, 0, NULL );
					}
					mx = my = 0;
				}
				else
#endif // HAVE_XF86DGA
				{
					// If it's a center motion, we've just returned from our warp
					if ( event.xmotion.x == window_width/2 && event.xmotion.y == window_height/2 )
					{
						mwx = window_width/2;
						mwy = window_height/2;
						if (t - mouseResetTime > MOUSE_RESET_DELAY )
						{
							Com_QueueEvent( t, SE_MOUSE, mx, my, 0, NULL );
						}
						mx = my = 0;
						break;
					}

					dx = ((int)event.xmotion.x - mwx);
					dy = ((int)event.xmotion.y - mwy);
					mx += dx;
					my += dy;
					mwx = event.xmotion.x;
					mwy = event.xmotion.y;
					dowarp = qtrue;
				} // if ( !in_dgamouse->value )
			} // if ( mouse_active )
			break;

		case ButtonPress:
		case ButtonRelease:
			if ( !IN_MouseActive() )
				break;

			if ( event.type == ButtonPress )
				btn_press = qtrue;
			else
				btn_press = qfalse;

			//t = Sys_XTimeToSysTime( event.xkey.time );
            t = Sys_Milliseconds();
			// NOTE TTimo there seems to be a weird mapping for K_MOUSE1 K_MOUSE2 K_MOUSE3 ..
			b = -1;
			switch ( event.xbutton.button )
			{
				case 1: b = 0; break; // K_MOUSE1
				case 2: b = 2; break; // K_MOUSE3
				case 3: b = 1; break; // K_MOUSE2
				case 4: Com_QueueEvent( t, SE_KEY, K_MWHEELUP, btn_press, 0, NULL ); break;
				case 5: Com_QueueEvent( t, SE_KEY, K_MWHEELDOWN, btn_press, 0, NULL ); break;
				case 6: b = 3; break; // K_MOUSE4
				case 7: b = 4; break; // K_MOUSE5
				case 8: case 9:       // K_AUX1..K_AUX8
				case 10: case 11:
				case 12: case 13:
				case 14: case 15:
					Com_QueueEvent( t, SE_KEY, event.xbutton.button - 8 + K_AUX1, btn_press, 0, NULL ); break;
			}
			if ( b != -1 ) // K_MOUSE1..K_MOUSE5
			{
				Com_QueueEvent( t, SE_KEY, K_MOUSE1 + b, btn_press, 0, NULL );
			}
			break; // case ButtonPress/ButtonRelease

		case CreateNotify:
			win_x = event.xcreatewindow.x;
			win_y = event.xcreatewindow.y;
			break;

		case ConfigureNotify:
			gw_minimized = WindowMinimized( dpy, win );
			Com_DPrintf( "ConfigureNotify gw_minimized: %i\n", gw_minimized );
			win_x = event.xconfigure.x;
			win_y = event.xconfigure.y;
			
			if ( !glw_state.cdsFullscreen && window_created && !gw_minimized )
			{
				Cvar_SetIntegerValue( "vid_xpos", win_x );
				Cvar_SetIntegerValue( "vid_ypos", win_y );
				RandR_UpdateMonitor( win_x, win_y,
					event.xconfigure.width,
					event.xconfigure.height );
			}
			Key_ClearStates();
			break;

		case FocusIn:
		case FocusOut:
			if ( event.type == FocusIn ) {
				window_focused = qtrue;
				Com_DPrintf( "FocusIn\n" );
			} else {
				window_focused = qfalse;
				Com_DPrintf( "FocusOut\n" );
			}
			Key_ClearStates();
			break;
		}

	}

	if ( dowarp )
	{
		XWarpPointer( dpy, None, win, 0, 0, 0, 0, window_width/2, window_height/2 );
	}
}


// NOTE TTimo for the tty console input, we didn't rely on those .. 
//   it's not very surprising actually cause they are not used otherwise
void KBD_Init( void )
{

}


void KBD_Close( void )
{

}



void IN_ActivateMouse( void )
{
	if ( !mouse_avail || !dpy || !win )
	{
		return;
	}

	if ( !mouse_active )
	{
		if ( in_dgamouse->integer && in_nograb->integer ) // force dga mouse to 0 if using nograb
		{
			Cvar_Set( "in_dgamouse", "0" );
		}
		install_mouse_grab();
		install_kb_grab();
		mouse_active = qtrue;
	}
}


/*
================
IN_DeactivateMouse
================
*/
void IN_DeactivateMouse( void )
{
	if ( !mouse_avail || !dpy || !win )
	{
		return;
	}

	if ( mouse_active )
	{
		uninstall_mouse_grab();
		uninstall_kb_grab();
		if ( in_dgamouse->integer && in_nograb->integer ) // force dga mouse to 0 if using nograb
		{
			Cvar_Set( "in_dgamouse", "0" );
		}
		mouse_active = qfalse;
	}
}


/*
================
IN_MouseActive
================
*/
qboolean IN_MouseActive( void )
{
	return ( in_nograb->integer == 0 && mouse_active );
}


/*
================
IN_Minimize
================
*/
void GLimp_Minimize( void )
{
/*   
	if ( !CL_VideoRecording() || ( re.CanMinimize && re.CanMinimize() ) )
	{
		XIconifyWindow( dpy, win, scrnum );
		XFlush( dpy );
	}
*/
    WindowMinimized( dpy, win );
}

/*****************************************************************************/
/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/
/*****************************************************************************/


void IN_Init( void )
{
	Com_Printf( "...Input Initialization...\n" );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_dgamouse = Cvar_Get( "in_dgamouse", "1", CVAR_ARCHIVE );
	in_shiftedKeys = Cvar_Get( "in_shiftedKeys", "0", CVAR_ARCHIVE );

	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get( "in_subframe", "1", CVAR_ARCHIVE );

	// developer feature, allows to break without loosing mouse pointer
	in_nograb = Cvar_Get( "in_nograb", "0", 0 );

	in_forceCharset = Cvar_Get( "in_forceCharset", "1", CVAR_ARCHIVE );
	Cmd_AddCommand( "in_restart", IN_Init );
	Cmd_AddCommand( "minimize", GLimp_Minimize );

	if ( in_mouse->integer )
	{
		mouse_avail = qtrue;
	}
	else
	{
		mouse_avail = qfalse;
	}
}


void IN_Shutdown( void )
{
	mouse_avail = qfalse;
}



void IN_Frame(void)
{
	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	qboolean loading = ( clc.state != CA_DISCONNECTED && clc.state != CA_ACTIVE );

	if( !cls.glconfig.isFullscreen && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !cls.glconfig.isFullscreen && loading )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !window_focused || gw_minimized || in_nograb->integer)
	{
		// Window not got focus
		IN_DeactivateMouse( );
	}
	else
		IN_ActivateMouse( );

/*     
	// Set event time for next frame to earliest possible time an event could happen
	in_eventTime = Sys_Milliseconds( );

	// In case we had to delay actual restart of video system
    if( ( vidRestartTime != 0 ) && ( vidRestartTime < Sys_Milliseconds( ) ) )
	{
		vidRestartTime = 0;
		Cbuf_AddText( "vid_restart\n" );
	}
*/    
}
