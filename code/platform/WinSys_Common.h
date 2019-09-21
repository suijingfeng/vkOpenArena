#ifndef WINSYS_COMMON_H_
#define WINSYS_COMMON_H_

void WinSys_ConstructDislayModes(void);
void WinSys_DestructDislayModes(void);

void ModeList_f( void );
qboolean CL_GetModeInfo( int *width, int *height, int mode, int dw, int dh, qboolean fullscreen );

#endif
