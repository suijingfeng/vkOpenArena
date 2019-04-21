#ifndef R_SORT_DRAW_SURFS_H_
#define R_SORT_DRAW_SURFS_H_

#include "tr_surface.h"


/*
the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

21 - 31	: sorted shader index
11 - 20	: entity index
2 - 6	: fog index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
0 - 1	: dlightmap index

	TTimo - 1.32
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
0-1   : dlightmap index
*/
#define	QSORT_FOGNUM_SHIFT      2
#define	QSORT_ENTITYNUM_SHIFT   7
#define	QSORT_SHADERNUM_SHIFT   17

void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs );

#endif
