#ifndef _X11_RANDR_H_
#define _X11_RANDR_H_

qboolean RandR_Init( int x, int y, int w, int h, int isFullScreen );
qboolean RandR_SetMode( int *width, int *height, int *rate );
void RandR_RestoreGamma( void );
void RandR_RestoreMode( void );
void RandR_Done( void );
#endif
