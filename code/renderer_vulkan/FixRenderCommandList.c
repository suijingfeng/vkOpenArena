#include "tr_globals.h"
#include "tr_shader.h"
#include "tr_cmds.h"
#include "R_SortDrawSurfs.h"
#include "ref_import.h"


// shader indexes from drawsurfs will be looked up in sortedShaders[]
static shader_t* L_SortedShaders[MAX_SHADERS];

void R_ClearSortedShaders(void)
{
    memset( L_SortedShaders, 0, sizeof(L_SortedShaders) );
}


void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap )
{
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = L_SortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_ENTITYNUM_SHIFT ) & (MAX_MOD_KNOWN - 1);
	*dlightMap = sort & 0x03;
}

static unsigned int R_ComposeSort(int sortedIndex, int entityNum, int fogNum, int dlightMap )
{
    return ((sortedIndex << QSORT_SHADERNUM_SHIFT) | (entityNum << QSORT_ENTITYNUM_SHIFT) | 
                            ( fogNum << QSORT_FOGNUM_SHIFT ) | dlightMap);
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList( int nShader )
{
    
    const void * pCmdTable = R_GetCommandBuffer(0);

    while ( 1 )
    {
        int cmd = *(const int *)pCmdTable;

        switch ( cmd )
        {
            case RC_SET_COLOR:
            {
                const setColorCommand_t *sc_cmd = (const setColorCommand_t *)pCmdTable;
                pCmdTable = (const void *)(sc_cmd + 1);
            } break;

            case RC_STRETCH_PIC:
            {
                const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)pCmdTable;
                pCmdTable = (const void *)(sp_cmd + 1);
            } break;

            case RC_DRAW_SURFS:
            {
                int i;
                drawSurf_t	*drawSurf;
                shader_t	*shader;
                int			fogNum;
                int			entityNum;
                int			dlightMap;
                
                const drawSurfsCommand_t *ds_cmd = (const drawSurfsCommand_t *)pCmdTable;

                for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; ++i, drawSurf++ )
                {
                    R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap );
                    int	sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
                    if( sortedIndex >= nShader )
                    {
                        sortedIndex++;
                        // why this entity not left shifted ???
                        drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | entityNum | 
                            ( fogNum << QSORT_FOGNUM_SHIFT ) | dlightMap;
                        
                        // drawSurf->sort = R_ComposeSort(sortedIndex, entityNum, fogNum, dlightMap);
                    }
                }

                pCmdTable = (const void *)(ds_cmd + 1);
            } break;
            case RC_DRAW_BUFFER:
            {
                const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)pCmdTable;
                pCmdTable = (const void *)(db_cmd + 1);
            } break;
            case RC_SWAP_BUFFERS:
            {
                const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)pCmdTable;
                pCmdTable = (const void *)(sb_cmd + 1);
            } break;

            // MUST BE return, otherwise loop forever;
            case RC_END_OF_LIST: return;
            default: return;
        }
    }
}

/*
==============
Positions the most recently created shader in the L_SortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
void R_SortNewShader( shader_t* pShader )
{
	int	i;
	float sort = pShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; --i )
    {
        // trival case
		if ( L_SortedShaders[ i ]->sort <= sort )
        {
			break;
		}

        // here is L_SortedShaders[ i ]->sort > sort
        // then all points one step back 
		L_SortedShaders[i+1] = L_SortedShaders[i];
		L_SortedShaders[i+1]->sortedIndex++;
	}

    // find the position, i

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i+1 );

	pShader->sortedIndex = i+1;
	L_SortedShaders[i+1] = pShader;
}


/*
typedef struct shader_s
{
    // ...
    int			numUnfoggedPasses;
	shaderStage_t* stages[MAX_SHADER_STAGES];		

    // ...
    struct shader_s *remappedShader;                  // current shader this one is remapped too

	struct	shader_s	*next;
} shader_t;
*/


shader_t* R_GeneratePermanentShader(shaderStage_t* pStgTab, shader_t* pSdr)
{
	
	int	i;

	if ( tr.numShaders == MAX_SHADERS )
    {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	shader_t* newShader = (shader_t*) ri.Hunk_Alloc( sizeof( shader_t ), h_low );
    // value-like copy, copy assign construstor, 
    // both pointers point to the same memory location
    *newShader = *pSdr;

	if ( newShader->sort <= SS_OPAQUE )
		newShader->fogPass = FP_EQUAL;
	else if ( newShader->contentFlags & CONTENTS_FOG )
		newShader->fogPass = FP_LE;
	
    // update index 
    newShader->index = tr.numShaders;
	newShader->sortedIndex = tr.numShaders;

	tr.shaders[ tr.numShaders ] = newShader;
	L_SortedShaders[ tr.numShaders ] = newShader;

	++tr.numShaders;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; ++i )
    {
		if ( !pStgTab[i].active ) {
			break;
		}
        
        // update pointer, to point to a new memory location
        newShader->stages[i] = 
            (shaderStage_t *) ri.Hunk_Alloc( sizeof(shaderStage_t), h_low );
        
        // struct assign, give it a new value
        // *(newShader->stages[i]) = pStgTab[i];
        *newShader->stages[i] = pStgTab[i];
        // very like c++ copy-assign constructor
/*
        shaderStage_t* pSS = newShader->stages[i]
        uint32_t b;
		for ( b = 0; b < NUM_TEXTURE_BUNDLES; ++b )
        {
            textureBundle_t* pTM = &pSS->bundle[b];

           
			uint32_t size = pTM->numTexMods * sizeof( texModInfo_t );

            // again, change the pointer to point another location
			pTM->texMods = (texModInfo_t*) ri.Hunk_Alloc( size, h_low );
			// copy old content to there
            memcpy( pTM->texMods, pStgTab[i].bundle[b].texMods, size );
		}
*/
	}

    // data already, sort it
	R_SortNewShader(tr.shaders[ tr.numShaders - 1 ]);
    
    R_UpdateShaderHashTable(newShader);

//    ri.Printf( PRINT_WARNING, "index:%d, newindex:%d, %s\n",
//            newShader->index, newShader->sortedIndex, newShader->name);
	
    return newShader;
}


/*
===============
Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void listSortedShader_f (void)
{
	int	i;
	int	count = tr.numShaders;

    ri.Printf (PRINT_ALL, " -------- %d shaders --------\n\n", count);

	for ( i = 0 ; i < count ; i++ )
    {
		ri.Printf( PRINT_ALL, "%i ", L_SortedShaders[i]->numUnfoggedPasses );

		if (L_SortedShaders[i]->lightmapIndex >= 0 )
			ri.Printf (PRINT_ALL, "L ");
		else
			ri.Printf (PRINT_ALL, "  ");


		if ( L_SortedShaders[i]->multitextureEnv == GL_ADD )
			ri.Printf( PRINT_ALL, "MT(a) " );
		else if ( L_SortedShaders[i]->multitextureEnv == GL_MODULATE )
			ri.Printf( PRINT_ALL, "MT(m) " );
		else
			ri.Printf( PRINT_ALL, "      " );
	

		if ( L_SortedShaders[i]->explicitlyDefined )
			ri.Printf( PRINT_ALL, "E " );
		else
			ri.Printf( PRINT_ALL, "  " );


		if ( L_SortedShaders[i]->isSky )
			ri.Printf( PRINT_ALL, "sky " );
		else
			ri.Printf( PRINT_ALL, "gen " );


		if ( L_SortedShaders[i]->defaultShader )
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", L_SortedShaders[i]->name);
		else
			ri.Printf (PRINT_ALL,  ": %s\n", L_SortedShaders[i]->name);
	}
	ri.Printf (PRINT_ALL, "-------------------\n");
}
