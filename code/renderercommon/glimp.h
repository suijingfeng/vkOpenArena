#ifndef GLIMP_H_
#define GLIMP_H_

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
////////////////////////////////////////////////////////////////



void GLimp_Init(glconfig_t *glConfig, qboolean coreContext);

void GLimp_Shutdown(void);
void GLimp_EndFrame(void);

void GLimp_LogComment(char *comment);
void GLimp_Minimize(void);

void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);

void GLimp_DeleteGLContext(void);
void GLimp_DestroyWindow(void);

void* GLimp_GetProcAddress(const char* fun);

#endif
