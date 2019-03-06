/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "input.h"
#include "glimp.h"
#include "../client/client.h"

#include <stdlib.h>
#include <stdio.h>
#include <xcb/xcb.h>

static int vidRestartTime = 0;
static int in_eventTime = 0;

static qboolean mouseActive = qfalse;
static qboolean mouseAvailable = qfalse;

static cvar_t *in_mouse = NULL;
static cvar_t *in_keyboardDebug = NULL;

static xcb_connection_t* connection_xcb;
static xcb_window_t window_xcb;
static xcb_intern_atom_reply_t *atom_wm_delete_window;

static cvar_t *in_nograb;



static int mwx, mwy;
static int mx = 0, my = 0;
static int window_width = 960;
static int window_height = 720;

void print_modifiers (uint32_t mask)
{
    const char **mod, *mods[] = {
        "Shift", "Lock", "Ctrl", "Alt",
        "Mod2", "Mod3", "Mod4", "Mod5",
        "Button1", "Button2", "Button3", "Button4", "Button5"
    };
    printf ("Modifier mask: ");
    for (mod = mods ; mask; mask >>= 1, mod++)
        if (mask & 1)
            printf(*mod);
    putchar ('\n');
}



#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of both of these methods
===============
*/
static qboolean IN_IsConsoleKey( keyNum_t key, int character )
{
	typedef struct consoleKey_s
	{
		enum
		{
			QUAKE_KEY,
			CHARACTER
		} type;

		union
		{
			keyNum_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	// Only parse the variable when it changes
	if( cl_consoleKeys->modified )
	{
		char *text_p;

		cl_consoleKeys->modified = qfalse;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;

		while( numConsoleKeys < MAX_CONSOLE_KEYS )
		{
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int charCode = 0;

			char* token = COM_ParseExt(&text_p, qtrue);
			if( !token[ 0 ] )
				break;

			if( strlen( token ) == 4 )
				charCode = Com_HexStrToInt( token );

			if( charCode > 0 )
			{
				c->type = CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type = QUAKE_KEY;
				c->u.key = Key_StringToKeynum( token );

				// 0 isn't a key
				if( c->u.key <= 0 )
					continue;
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if( key == character )
		key = 0;

	for( i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch( c->type )
		{
			case QUAKE_KEY:
				if( key && c->u.key == key )
					return qtrue;
				break;

			case CHARACTER:
				if( c->u.character == character )
					return qtrue;
				break;
		}
	}

	return qfalse;
}



static void IN_ActivateMouse( void )
{

    xcb_cursor_t cursor = XCB_NONE;

	if( !mouseActive )
	{
/*
        // Grabs the pointer actively
        uint32_t mask =  XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
                XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW   |
                XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE;


        xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(
                connection_xcb,
                mask,                // get all pointer events specified by the following mask
                window_xcb,          // grab the root window
                XCB_NONE,            // which events to let through
                XCB_GRAB_MODE_ASYNC, // pointer events should continue as normal
                XCB_GRAB_MODE_ASYNC, // keyboard mode
                window_xcb,          // confine_to = in which window should the cursor stay
                cursor,              // we change the cursor to whatever the user wanted
                XCB_CURRENT_TIME
                );

        xcb_grab_pointer_reply_t *pReply = xcb_grab_pointer_reply(connection_xcb, cookie, NULL);

        if (pReply)
        {
            if (pReply->status == XCB_GRAB_STATUS_SUCCESS)
                printf("successfully grabbed the pointer\n");
            else
                Com_Error(ERR_FATAL, "failed grabbed the pointer.\n");
            free(pReply);
        }
*/
        // move pointer to destination window area
	    xcb_warp_pointer( connection_xcb, XCB_NONE, window_xcb, 0, 0, 0, 0, window_width / 2, window_height / 2 );

		xcb_set_input_focus(connection_xcb, XCB_INPUT_FOCUS_POINTER_ROOT, window_xcb, XCB_CURRENT_TIME);

		mwx = window_width / 2;
		mwy = window_height / 2;
		mx = my = 0;

    	mouseActive = qtrue;
    }

}


static void IN_DeactivateMouse(void)
{
	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if( !mouseAvailable )
		return;

	if( mouseActive )
	{
        xcb_ungrab_pointer( connection_xcb, XCB_TIME_CURRENT_TIME);
		mouseActive = qfalse;
	}
}


/*
   If a user clicks a mouse button, the message from the X server can be accessed
   as a xcb_button_press_event_t, which contains fields related to the mouse event.
   Its fields include the following:

    detail ！ the button pressed
    time ！ the timestamp of the mouse click
    event_x, event_y ！ event coordinates relative to the client window
    root_x, root_y ！ event coordinates relative to the root window
    root ！ ID of the root window
    child ！ ID of the child window

    detail is a value of the xcb_button_index_t enumerated type, which has six values:

    XCB_BUTTON_INDEX_ANY ！ any mouse button
    XCB_BUTTON_INDEX_1 ！ the left mouse button
    XCB_BUTTON_INDEX_2 ！ the middle mouse button
    XCB_BUTTON_INDEX_3 ！ the right mouse button
    XCB_BUTTON_INDEX_4 ！ mouse scroll up
    XCB_BUTTON_INDEX_5 ！ mouse scroll down
*/



static void IN_ProcessEvents( void )
{
	xcb_generic_event_t *e;
	keyNum_t key = 0;
	static keyNum_t lastKeyDown = 0;

	while( (e = xcb_poll_for_event(connection_xcb)) )
	{
		switch( e->response_type & ~0x80 )
		{
            case XCB_EXPOSE:
            {
                break;
            }
            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *ev = (xcb_button_press_event_t *)e;
       
                switch (ev->detail)
                {
                    case XCB_BUTTON_INDEX_1:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE1, qtrue, 0, NULL );
                        break; //  the left mouse button
                    case XCB_BUTTON_INDEX_2:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE3, qtrue, 0, NULL );
                        break; //  the middle mouse button
                    case XCB_BUTTON_INDEX_3:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE2, qtrue, 0, NULL );
                        break; //  the right mouse button
                    case XCB_BUTTON_INDEX_4:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
                        break; //  mouse scroll up
                    case XCB_BUTTON_INDEX_5:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
                        break;//  mouse scroll down
                    default:
                        printf ("Button %d pressed in window %ld, at coordinates (%d,%d)\n",
                                ev->detail, ev->event, ev->event_x, ev->event_y);
                }

                break;
            }
            case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t *ev = (xcb_button_release_event_t *)e;
                    
                switch (ev->detail)
                {
                    case XCB_BUTTON_INDEX_1:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
                        break; //  the left mouse button
                    case XCB_BUTTON_INDEX_2:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE3, qfalse, 0, NULL );
                        break; //  the middle mouse button
                    case XCB_BUTTON_INDEX_3:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE2, qfalse, 0, NULL );
                        break; //  the right mouse button
                    case XCB_BUTTON_INDEX_4:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
                        break; //  mouse scroll up
                    case XCB_BUTTON_INDEX_5:
                        Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
                        break;//  mouse scroll down
                    default:
                        printf ("Button %d pressed in window %ld, at coordinates (%d,%d)\n",
                                ev->detail, ev->event, ev->event_x, ev->event_y);
                }
                break;
            }
            case XCB_MOTION_NOTIFY:
            {

                xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)e;

                if( mouseActive )
				{
					if( !ev->event_x && !ev->event_y )
						break;

					int dx = ((int)ev->event_x - mwx);
					int dy = ((int)ev->event_y - mwy);
					mx += dx;
					my += dy;
					mwx = ev->event_x;
					mwy = ev->event_y;

                    Com_QueueEvent( in_eventTime, SE_MOUSE, mx, my, 0, NULL );

                    xcb_warp_pointer( connection_xcb, XCB_NONE, window_xcb, 0, 0, 0, 0, window_width/2, window_height/2 ); 
				}
                break;
            }
            case XCB_ENTER_NOTIFY:
            {
                xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)e;

                printf ("Mouse entered window %ld, at coordinates (%d,%d)\n",
                        ev->event, ev->event_x, ev->event_y);
                break;
            }
            case XCB_LEAVE_NOTIFY:
            {
                xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)e;

                printf ("Mouse left window %ld, at coordinates (%d,%d)\n",
                        ev->event, ev->event_x, ev->event_y);
                break;
            }
            case XCB_KEY_PRESS:
            {
                xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;

                key = ev->detail; 
				Com_QueueEvent(in_eventTime, SE_KEY, key, qtrue, 0, NULL );

				if( key == K_BACKSPACE )
					Com_QueueEvent(in_eventTime, SE_CHAR, ('h'-'a'+1), 0, 0, NULL );
				else if( keys[K_CTRL].down && key >= 'a' && key <= 'z' )
					Com_QueueEvent(in_eventTime, SE_CHAR, (key-'a'+1), 0, 0, NULL );

                break;
            }
            case XCB_KEY_RELEASE:
            {
                xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;
                key = ev->detail;
                Com_QueueEvent(in_eventTime, SE_KEY, key, qfalse, 0, NULL );

                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                if ((*(xcb_client_message_event_t *)e).data.data32[0] ==
                        (*atom_wm_delete_window).atom)
                {
                	Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
				
                }
                break;
            }

            default:
                /* Unknown event type, ignore it */
                printf("Unknown event: %d\n", e->response_type);
                break;
        }
        /* Free the Generic Event */
        free (e);
    }
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
	else
		IN_ActivateMouse( );

	// Set event time for next frame to earliest possible time an event could happen
	in_eventTime = Sys_Milliseconds( );

	IN_ProcessEvents( );


	// In case we had to delay actual restart of video system
    if( ( vidRestartTime != 0 ) && ( vidRestartTime < Sys_Milliseconds( ) ) )
	{
		vidRestartTime = 0;
		Cbuf_AddText( "vid_restart\n" );
	}
}


void IN_Init(void* connection, unsigned int win)
{
    Com_Printf("...IN_Init...\n");

    connection_xcb = (xcb_connection_t*) connection;
    window_xcb = win;

    xcb_intern_atom_cookie_t cookie2
        = xcb_intern_atom(connection_xcb, 0, 16, "WM_DELETE_WINDOW");
    
    atom_wm_delete_window = xcb_intern_atom_reply(connection_xcb, cookie2, 0);

	in_keyboardDebug = Cvar_Get("in_keyboardDebug", "0", CVAR_ARCHIVE);
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	mouseAvailable = ( in_mouse->value != 0 );
	//SDL_StartTextInput( );

	IN_DeactivateMouse( );


}

void IN_Restart( void )
{
   IN_Init(connection_xcb, window_xcb); 
}


void IN_Shutdown(void)
{
//	SDL_StopTextInput();

	IN_DeactivateMouse();
    mouseAvailable = qfalse;
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
