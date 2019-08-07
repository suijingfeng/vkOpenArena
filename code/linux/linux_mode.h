#ifndef LINUX_MODE_H_
#define LINUX_MODE_H_

int R_GetModeInfo(int * const width, int * const height, int mode, const int desktopWidth, const int desktopHeight);

void win_InitDisplayModel(void);
void win_EndDisplayModel(void);


#endif
