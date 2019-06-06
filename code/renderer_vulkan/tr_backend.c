#include "ref_import.h"
#include "tr_backend.h"
#include "R_PrintMat.h"

backEndState_t backEnd;


void R_ClearBackendState(void)
{
    ri.Printf(PRINT_ALL, " backend state cleared. \n");
	// clear all our internal state
	memset( &backEnd, 0, sizeof( backEnd ) );
}


void R_PrintBackEnd_OR_f(void)
{
    // in world coordinates
    printMat1x3f("backEnd.or.origin", backEnd.or.origin);
    // orientation in world
    printMat3x3f("backEnd.or.axis", backEnd.or.axis);
    // viewParms->or.origin in local coordinates
    printMat1x3f("backEnd.or.viewOrigin", backEnd.or.viewOrigin);
    printMat4x4f("backEnd.or.modelMatrix", backEnd.or.modelMatrix);	
}

uint32_t R_GetRefRdTime(void)
{
    return backEnd.refdef.time;
}

void R_UpdatePerformanceCounters(uint32_t nVerts, uint32_t nIdxes, uint32_t nPasses)
{
	++backEnd.pc.c_shaders;
	backEnd.pc.c_vertexes += nVerts;
	backEnd.pc.c_indexes += nIdxes;
	backEnd.pc.c_totalIndexes += nIdxes * nPasses;
}
