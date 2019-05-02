#include "tr_shader.h"

#include "tr_cmds.h" // R_AddDrawSurfCmd
#include "R_Portal.h"
#include "ref_import.h"

#include "tr_globals.h"
#include "tr_shader.h"
#include "tr_cvar.h"

#include "FixRenderCommandList.h"
#include "R_GetMicroSeconds.h"

/*
==========================================================================

DRAWSURF SORTING

==========================================================================
*/


/* 
 * this parameter defines the cutoff between using quick sort and
   insertion sort for arrays; arrays with lengths shorter or equal to the
   below value use insertion sort 

   testing shows that this is good value
*/
#define CUTOFF  8


static void Algo_SwapDrawSurf(struct drawSurf_s* const base, int left, int right)
{
    struct drawSurf_s tmp = base[left];
    base[left] = base[right];
    base[right] = tmp;
}


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


void Algo_QuickSort( drawSurf_t* const v, int left, int right)
{
	int i, last;

	if(left >= right) // do nothing if array contains fewer than two elements
		return;

    if (left + CUTOFF < right)
    {
        Algo_SwapDrawSurf(v, left, (left+right)/2);
	
        last = left;
        for(i = left+1; i<=right; ++i)
            if(v[i].sort < v[left].sort)
                Algo_SwapDrawSurf(v, ++last, i);

        Algo_SwapDrawSurf(v, left, last);

        Algo_QuickSort(v, left, last-1);
        Algo_QuickSort(v, last+1, right);
    }
    else
        Algo_InsertSort(v, left, right);
}



/*
===============
R_Radix
===============
*/
static void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest )
{
  int           count[ 256 ] = { 0 };
  int           index[ 256 ];
  int           i;
  unsigned char *sortKey = NULL;
  unsigned char *end = NULL;

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  end = sortKey + ( size * sizeof( drawSurf_t ) );
  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    ++count[ *sortKey ];

  index[ 0 ] = 0;

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
===============
R_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void R_RadixSort( drawSurf_t *source, int size )
{
  static drawSurf_t scratch[ MAX_DRAWSURFS ];
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




void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
        ri.Printf(PRINT_WARNING, " numDrawSurfs overflowed. \n");
	}
    
    // ri.Printf(PRINT_WARNING, " numDrawSurfs:%d \n", numDrawSurfs);
	
    
    // uint64_t start = R_GetTimeMicroSeconds();
    // sort the drawsurfs by sort type, then orientation, then shader
    
    // 85 us
    // Algo_QuickSort(drawSurfs, 0, numDrawSurfs - 1);
    // ORIGINAL
    // qsortFast (drawSurfs, numDrawSurfs, sizeof(drawSurf_t) );
    //
    // fastest 20-30
    R_RadixSort (drawSurfs, numDrawSurfs);


//    if(numDrawSurfs > 10)
//    {
//        ri.Printf(PRINT_WARNING, " numDrawSurfs: %ld us \n", R_GetTimeMicroSeconds() - start);
//    }

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0 ; i < numDrawSurfs ; i++ )
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
