#ifndef WINSYS_COMMON_H_
#define WINSYS_COMMON_H_

void WinSys_ConstructDislayModes(void);
void WinSys_DestructDislayModes(void);

// void FileSys_Logging(const char * const comment );


qboolean CL_GetModeInfo( int *width, int *height, int mode, int dw, int dh, qboolean fullscreen );
int R_GetDisplayMode(int mode, uint32_t * const pWidth, uint32_t * const pHeight);
#endif
