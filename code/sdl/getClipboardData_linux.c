#include <stdio.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "local.h"
#include "public.h"
#include "input.h"

extern Display * dpy;
extern Window win;

char *Sys_GetClipboardData( void )
{
	const Atom xtarget = XInternAtom( dpy, "UTF8_STRING", 0 );
	unsigned long nitems, rem;
	unsigned char *data;
	Atom type;
	XEvent ev;
	char *buf;
	int format;

	XConvertSelection( dpy, XA_PRIMARY, xtarget, XA_PRIMARY, win, CurrentTime );
	XSync( dpy, False );
	XNextEvent( dpy, &ev );
	if ( !XFilterEvent( &ev, None ) && ev.type == SelectionNotify ) {
		if ( XGetWindowProperty( dpy, win, XA_PRIMARY, 0, 8, False, AnyPropertyType,
			&type, &format, &nitems, &rem, &data ) == 0 ) {
			if ( format == 8 ) {
				if ( nitems > 0 ) {
					buf = Z_Malloc( nitems + 1 );
					Q_strncpyz( buf, (char*)data, nitems + 1 );
					strtok( buf, "\n\r\b" );
					return buf;
				}
			} else {
				fprintf( stderr, "Bad clipboard format %i\n", format );
			}
		} else {
			fprintf( stderr, "Clipboard allocation failed\n" );
		}
	}
	return NULL;
}
