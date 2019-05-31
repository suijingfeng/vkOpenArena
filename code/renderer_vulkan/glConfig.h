#ifndef GL_CONFIGURE_H_
#define GL_CONFIGURE_H_

#include "../renderercommon/tr_types.h"  // glconfig_t

void R_SetWinMode(int mode, unsigned int w, unsigned int h, unsigned int hz);

void R_DisplayResolutionList_f(void);
void R_InitDisplayResolution( void );


void glConfig_Init(void);
void glConfig_FillString( void );
void glConfig_Get(glconfig_t * const pOut);
void glConfig_Clear(void);

uint32_t vk_getWinWidth(void);
uint32_t vk_getWinHeight(void);

#endif
