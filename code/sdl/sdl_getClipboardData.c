
#ifdef _WIN32
	#include "../SDL2/include/SDL.h"
#else
	#include <SDL2/SDL.h>
#endif
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

/*
 * This makes pasting in client console and UI edit fields work on X11 and OS X.
 * Sys_GetClipboardData is only used by client, so returning NULL in dedicated is fine.
 */
char *Sys_GetClipboardData(void)
{
#ifdef DEDICATED
	return NULL;
#else
	char *data = NULL;
	char *cliptext = SDL_GetClipboardText();

	if( cliptext != NULL )
    {
		if ( cliptext[0] != '\0' )
        {
			size_t bufsize = strlen( cliptext ) + 1;

			data = Z_Malloc( bufsize );
			Q_strncpyz( data, cliptext, bufsize );

			// find first listed char and set to '\0'
			strtok( data, "\n\r\b" );
		}
		SDL_free( cliptext );
	}
	return data;
#endif
}
