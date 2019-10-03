#include <X11/Xatom.h>

#include "sys_public.h"
#include "win_public.h"

extern WinVars_t glw_state;

char *Sys_GetClipboardData( void )
{
	const Atom xtarget = XInternAtom( glw_state.pDisplay, "UTF8_STRING", 0 );
	unsigned long nitems, rem;
	unsigned char *data;
	Atom type;
	XEvent ev;
	char *buf;
	int format;

	XConvertSelection( glw_state.pDisplay, XA_PRIMARY, xtarget, XA_PRIMARY, glw_state.hWnd, CurrentTime );
	XSync( glw_state.pDisplay, False );
	XNextEvent( glw_state.pDisplay, &ev );
	if ( !XFilterEvent( &ev, None ) && ev.type == SelectionNotify )
    {
		if ( XGetWindowProperty( glw_state.pDisplay, glw_state.hWnd, XA_PRIMARY, 0, 8, False, AnyPropertyType,
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
