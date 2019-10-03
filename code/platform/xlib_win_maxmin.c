#include <X11/Xatom.h>

#include "sys_public.h"
#include "win_public.h"


extern WinVars_t glw_state;


static qboolean WindowMinimized( void )
{
	unsigned long i, num_items, bytes_after;
	Atom actual_type, *atoms, nws, nwsh;
	int actual_format;

	nws = XInternAtom( glw_state.pDisplay, "_NET_WM_STATE", True );
	if ( nws == BadValue || nws == None )
		return qfalse;

	nwsh = XInternAtom( glw_state.pDisplay, "_NET_WM_STATE_HIDDEN", True );
	if ( nwsh == BadValue || nwsh == None )
		return qfalse;

	atoms = NULL;

	XGetWindowProperty( glw_state.pDisplay, glw_state.hWnd, nws, 0, 0x7FFFFFFF, False, XA_ATOM,
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


void WinMinimize_f(void)
{
    glw_state.isMinimized = WindowMinimized( );
    Com_Printf( " gw_minimized: %i\n", glw_state.isMinimized );
}
