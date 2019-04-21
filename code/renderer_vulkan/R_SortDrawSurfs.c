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
/*
    if (right - left <= CUTOFF)
    {
         Algo_InsertSort(v, left, right);
    }
*/
	Algo_SwapDrawSurf(v, left, (left+right)/2);
	
	last = left;
	for(i = left+1; i<=right; ++i)
		if(v[i].sort < v[left].sort)
			Algo_SwapDrawSurf(v, ++last, i);

	Algo_SwapDrawSurf(v, left, last);

	Algo_QuickSort(v, left, last-1);
	Algo_QuickSort(v, last+1, right);
}


static void SWAP_DRAW_SURF( drawSurf_t *a, drawSurf_t* b )
{
    drawSurf_t tmp = *a;
    *a = *b;
    *b = tmp;
/*
    char buf[sizeof(drawSurf_t)];
    memcpy(buf, a, sizeof(drawSurf_t));
    memcpy(a, b, sizeof(drawSurf_t));
    memcpy(b, buf, sizeof(drawSurf_t));
*/
}


static void shortsort( drawSurf_t * const lo, drawSurf_t * hi )
{
    while (hi > lo)
    {
        drawSurf_t * max = lo;
        drawSurf_t * p;
        for (p = lo + 1; p <= hi; ++p)
        {
            if ( p->sort > max->sort )
            {
                max = p;
            }
        }
        SWAP_DRAW_SURF(max, hi);
        --hi;
    }
}



/* sort the array between lo and hi (inclusive)
FIXME: this was lifted and modified from the microsoft lib source...
 */

static void qsortFast ( drawSurf_t * base,  unsigned num, unsigned width )
{
            
    char *mid;                  /* points to middle of subarray */
    char *loguy, *higuy;        /* traveling pointers for partition step */
    unsigned size;              /* size of the sub-array */
    char *lostk[30], *histk[30];
    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2 || width == 0)
        return;                 /* nothing to do */
    
    /* stack for saving sub-array to be processed */
    int stkptr = 0;                 /* initialize stack */
    
    /* ends of sub-array currently sorting */
    char * lo = (char *)base;
    char * hi = (char *)(base + (num-1));
    /* initialize limits */

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    size = (hi - lo) / width + 1;        /* number of el's to sort */

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= CUTOFF) {
         shortsort((drawSurf_t *)lo, (drawSurf_t *)hi);
    }
    else
    {
        /* First we pick a partititioning element.  The efficiency of the
           algorithm demands that we find one that is approximately the
           median of the values, but also that we select one fast.  Using
           the first one produces bad performace if the array is already
           sorted, so we use the middle one, which would require a very
           wierdly arranged array for worst case performance.  Testing shows
           that a median-of-three algorithm does not, in general, increase
           performance. */

        mid = lo + (size / 2) * width;      /* find middle element */
        SWAP_DRAW_SURF((drawSurf_t *)mid, (drawSurf_t *)lo); /* swap it to beginning of array */

        /* We now wish to partition the array into three pieces, one
           consisiting of elements <= partition element, one of elements
           equal to the parition element, and one of element >= to it.  This
           is done below; comments indicate conditions established at every
           step. */

        loguy = lo;
        higuy = hi + width;

        /* Note that higuy decreases and loguy increases on every iteration,
           so loop must terminate. */
        for (;;)
        {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do  {
                loguy += width;
            } while (loguy <= hi &&  
				( ((drawSurf_t *)loguy)->sort <= ((drawSurf_t *)lo)->sort ) );

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do  {
                higuy -= width;
            } while (higuy > lo && 
				( ((drawSurf_t *)higuy)->sort >= ((drawSurf_t *)lo)->sort ) );

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            SWAP_DRAW_SURF((drawSurf_t *)loguy, (drawSurf_t *)higuy);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        SWAP_DRAW_SURF((drawSurf_t *)lo, (drawSurf_t *)higuy);     /* put partition element in place */

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if ( higuy - 1 - lo >= hi - loguy )
        {
            if (lo + width < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else
        {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo + width < higuy) {
                hi = higuy - width;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
       Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
    else
        return;                 /* all subarrays done */
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
	
    
    uint64_t start = R_GetTimeMicroSeconds();
    // sort the drawsurfs by sort type, then orientation, then shader
    
    // 85 us
    // Algo_QuickSort(drawSurfs, 0, numDrawSurfs - 1);
    // ORIGINAL
    // qsortFast (drawSurfs, numDrawSurfs, sizeof(drawSurf_t) );
    //
    // try slow algorithm, still run, but about 1000ms
    // shortsort(drawSurfs, drawSurfs + numDrawSurfs - 1);
    //
    // fastest 20-30
    R_RadixSort (drawSurfs, numDrawSurfs);

    if(numDrawSurfs > 10)
    {
        ri.Printf(PRINT_WARNING, " numDrawSurfs: %ld us \n", R_GetTimeMicroSeconds() - start);
    }

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