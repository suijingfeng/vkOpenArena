#ifndef R_FIND_SHADER_H_
#define R_FIND_SHADER_H_

#include "tr_shader.h"

qhandle_t RE_RegisterShader( const char *name );
qhandle_t RE_RegisterShaderNoMip( const char *name );

shader_t* R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
void ScanAndLoadShaderFiles( void );
#endif
