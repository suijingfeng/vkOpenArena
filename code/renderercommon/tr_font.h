#ifndef TR_FONT_H_
#define TR_FONT_H_

#include "../qcommon/q_shared.h"


extern qhandle_t RE_RegisterShaderNoMip( const char *name );

void R_InitFreeType( void );
void R_DoneFreeType( void );
void RE_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);


#endif
