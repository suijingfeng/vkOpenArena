#include "tr_local.h"

#include "R_GetMicroSeconds.h"

extern qboolean R_MirrorViewBySurface(drawSurf_t *drawSurf, int entityNum);



/*
==========================================================================

DRAWSURF SORTING

==========================================================================
*/



// swap data
static inline void Algo_SwapDrawSurf(struct drawSurf_s* const base, int left, int right)
{
    struct drawSurf_s tmp = base[left];
    base[left] = base[right];
    base[right] = tmp;
}


// insert sort
static void Algo_InsertSort( drawSurf_t* const v, int left, int right)
{
    while (right > left)
    {
        int max = left;
        for (int i = left + 1; i <= right; ++i)
        {
            if ( v[i].sort > v[max].sort )
            {
                max = i;
            }
        }
        Algo_SwapDrawSurf(v, max, right);
        --right;
    }
}


// pure quick sort  
void Algo_QuickSort( drawSurf_t* const v, int left, int right)
{
	if(left >= right) // do nothing if array contains fewer than two elements
		return;

    Algo_SwapDrawSurf(v, left, (left+right)/2);
	
    int last = left;
    for(int i = left+1; i<=right; ++i)
        if(v[i].sort < v[left].sort)
            Algo_SwapDrawSurf(v, ++last, i);

    Algo_SwapDrawSurf(v, left, last);

    Algo_QuickSort(v, left, last-1);
    Algo_QuickSort(v, last+1, right);
}


// quick sort combine with insert sort
// not get big improvement
static void Algo_QuickSort2(drawSurf_t* const v, int left, int right)
{
//
//	this parameter defines the cutoff between using quick sort and
//	insertion sort for arrays; arrays with lengths shorter or equal
//	to the below value use insertion sort,  testing shows that this
//	is good value


#define CUTOFF  8

	if (left >= right) // do nothing if array contains fewer than two elements
		return;

	if (left + CUTOFF < right)
	{
		Algo_SwapDrawSurf(v, left, (left + right) / 2);

		int last = left;
		for (int i = left + 1; i <= right; ++i)
			if (v[i].sort < v[left].sort)
				Algo_SwapDrawSurf(v, ++last, i);

		Algo_SwapDrawSurf(v, left, last);

		Algo_QuickSort(v, left, last - 1);
		Algo_QuickSort(v, last + 1, right);
	}
	else
       Algo_InsertSort(v, left, right);

#undef CUTOFF
}

/*
===============
R_Radix
===============
*/
static void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest )
{
	int count[256] = { 0 };
	int index[256] = { 0 };

	unsigned char *sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
	unsigned char *end = sortKey + ( size * sizeof( drawSurf_t ) );
	for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
		++count[ *sortKey ];


	for(int i = 1; i < 256; ++i )
		index[ i ] = index[ i - 1 ] + count[ i - 1 ];

	sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
	for(int i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
===============
Algo__RadixSort

Radix sort with 4 byte size buckets

best for speed, but using 65536 static item
and have relationship with ENDIAN ?
===============
*/


static void Algo_RadixSort( drawSurf_t * const source, const int size )
{
#define Q3_LITTLE_ENDIAN 1

	static drawSurf_t scratch[MAX_DRAWSURFS];
#ifdef Q3_LITTLE_ENDIAN
	R_Radix( 0, size, source, scratch );
	R_Radix( 1, size, scratch, source );
	R_Radix( 2, size, source, scratch );
	R_Radix( 3, size, scratch, source );
#else
	R_Radix( 3, size, source, scratch );
	R_Radix( 2, size, scratch, source );
	R_Radix( 1, size, source, scratch );
	R_Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN

}


/*
void R_SortDrawSurfs2( drawSurf_t * const drawSurfs, const int numDrawSurfs )
{
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 2 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	//
	// if ( numDrawSurfs > MAX_DRAWSURFS ) {
	//	numDrawSurfs = MAX_DRAWSURFS;
	//    ri.Printf(PRINT_WARNING, " numDrawSurfs overflowed. \n");
	// }

	// ri.Printf(PRINT_WARNING, " numDrawSurfs:%d \n", numDrawSurfs);


	// uint64_t start = R_GetTimeMicroSeconds();
	// sort the drawsurfs by sort type, then orientation, then shader

	// 85 us
	// Algo_QuickSort(drawSurfs, 0, numDrawSurfs - 1);
	// ORIGINAL
	// qsortFast (drawSurfs, numDrawSurfs, sizeof(drawSurf_t) );
	//
	// fastest 20-30

	// ri.Printf(PRINT_WARNING, " numDrawSurfs: %d \n", numDrawSurfs);

	Algo_RadixSort (drawSurfs, numDrawSurfs);


//    if(numDrawSurfs > 10)
//    {
//        ri.Printf(PRINT_WARNING, " numDrawSurfs: %ld us \n", R_GetTimeMicroSeconds() - start);
//    }

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0 ; i < numDrawSurfs ; ++i )
	{
		R_DecomposeSort( (drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			ri.Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( (drawSurfs+i), entityNum) )
		{
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}
*/

void R_SortDrawSurfs(drawSurf_t * const drawSurfs, int numDrawSurfs)
{
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;

	// it is possible for some views to not have any surfaces
	if (numDrawSurfs < 1) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if (numDrawSurfs > MAX_DRAWSURFS) {
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	// qsortFast (drawSurfs, numDrawSurfs, sizeof(drawSurf_t) );


//	uint64_t start = R_GetTimeMicroSeconds();
	
	// RadixSort two time faster than QuickSort
	// typically  40us vs 80 us on AMD 2700x
 	Algo_RadixSort(drawSurfs, numDrawSurfs);
	// Algo_QuickSort(drawSurfs, 0, numDrawSurfs - 1);
/*
	if(numDrawSurfs > 10)
    {
        ri.Printf(PRINT_WARNING, " numDrawSurfs: %ld us \n",
			R_GetTimeMicroSeconds() - start);
    }
*/
	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for (i = 0; i < numDrawSurfs; ++i)
	{
		R_DecomposeSort((drawSurfs + i)->sort, &entityNum, &shader, &fogNum, &dlighted);

		if (shader->sort > SS_PORTAL) {
			break;
		}

		// no shader should ever have this sort type
		if (shader->sort == SS_BAD) {
			ri.Error(ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name);
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if (R_MirrorViewBySurface((drawSurfs + i), entityNum)) {
			// this is a debug option to see exactly what is being mirrored
			if (r_portalOnly->integer) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
}
