#ifndef SDL_GLIMP_H
#define SDL_GLIMP_H

#ifdef USE_LOCAL_HEADERS
#include "SDL.h"
#else
#include <SDL.h>
#endif


/*
 * IMPLEMENTATION SPECIFIC FUNCTIONS
 */

void GLimp_Init( qboolean );
void GLimp_Shutdown( void );
void GLimp_EndFrame( void );

void GLimp_LogComment( char *comment );
void GLimp_Minimize(void);

void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256] );

#endif
