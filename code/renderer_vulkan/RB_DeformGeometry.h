#ifndef RB_DEFORM_GEOMETRY
#define RB_DEFORM_GEOMETRY

#include "R_ShaderCommands.h"

void RB_DeformTessGeometry(shaderCommands_t * const pTess);

float RB_WaveValue(enum GenFunc_T func, const float base, 
        const float Amplitude, const float phase, const float freq );
#endif
