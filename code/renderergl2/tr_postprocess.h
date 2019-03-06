/*
===========================================================================
Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete

This file is part of Reaction source code.

Reaction source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Reaction source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Reaction source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef TR_POSTPROCESS_H
#define TR_POSTPROCESS_H

#include "tr_fbo.h"

void RB_ToneMap(FBO_t *hdrFbo, int hdrBox[4], FBO_t *ldrFbo, int ldrBox[4], int autoExposure);
void RB_BokehBlur(FBO_t *src, int srcBox[4], FBO_t *dst, int dstBox[4], float blur);
void RB_SunRays(FBO_t *srcFbo, int srcBox[4], FBO_t *dstFbo, int dstBox[4]);
void RB_GaussianBlur(float blur);

#endif
