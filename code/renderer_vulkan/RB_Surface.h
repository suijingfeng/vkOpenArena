#ifndef RB_SURFACE_H_
#define RB_SURFACE_H_

#define RB_CHECKOVERFLOW(v,i)                                   \
    if ( (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES) ||     \
         (tess.numIndexes + (i) >= SHADER_MAX_INDEXES) ) {      \
        RB_CheckOverflow(v,i);                                  \
    }

void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );

#endif
