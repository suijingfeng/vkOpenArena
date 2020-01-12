#ifndef R_FIND_SHADER_H_
#define R_FIND_SHADER_H_

#include "tr_shader.h"

qhandle_t RE_RegisterShader( const char *name );
qhandle_t RE_RegisterShaderNoMip( const char *name );

struct shader_s * R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
void R_UpdateShaderHashTable(struct shader_s * newShader);
void ScanAndLoadShaderFiles( void );
#endif
