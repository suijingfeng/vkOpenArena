#ifndef GL_CONFIGURE_H_
#define GL_CONFIGURE_H_

#include "../renderercommon/tr_types.h"  // glconfig_t

void R_SetWinMode(int mode, unsigned int w, unsigned int h, unsigned int hz);

void R_DisplayResolutionList_f(void);
void R_InitDisplayResolution( void );


void glConfig_Init(void);
void glConfig_FillString( void );
void glConfig_Get(glconfig_t * const pOut);
// temporary ...
glconfig_t * glConfig_getAddressOf(void);
void glConfig_Clear(void);


#endif
