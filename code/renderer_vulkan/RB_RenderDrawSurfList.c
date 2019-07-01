#include "tr_globals.h" //tr.worldEn
#include "tr_backend.h"
#include "tr_surface.h"
#include "tr_light.h"
#include "R_ShaderCommands.h"   //tess
#include "R_RotateForViewer.h"
#include "vk_shade_geometry.h"
#include "vk_frame.h"

#include "FixRenderCommandList.h"

extern struct shaderCommands_s tess;


void RB_RenderDrawSurfList(const struct drawSurf_s * const pSurf, const uint32_t numDrawSurfs, 
        const trRefdef_t * const pRefdef, const viewParms_t * const pViewPar )
{

    backEnd.refdef = *pRefdef;
    backEnd.viewParms = *pViewPar;
	backEnd.projection2D = qfalse;

	// save original time for entity shader offsets
	float originalTime = R_GetRefFloatTime();

    // Any mirrored or portaled views have already been drawn, 
    // so prepare to actually render the visible surfaces for this view
	// clear the z buffer, set the modelview, etc
	// RB_BeginDrawingView ();
    //

	// VULKAN

	// we will need to change the projection matrix before drawing
	// 2D images again

    if ( backEnd.refdef.rdflags & RDF_HYPERSPACE )
	{
		//RB_Hyperspace();
        // A player has predicted a teleport, but hasn't arrived yet

        const float c = ( backEnd.refdef.time & 255 ) / 255.0f;
        const float color[4] = { c, c, c, 1.0f };

        // so short, do we really need this?
	    vk_clearColorAttachments(color);
	}
    

    
	// draw everything

	int oldEntityNum = -1;
	int oldFogNum = -1;
	int oldDlighted = qfalse;
	unsigned int oldSort = -1;

    shader_t* oldShader = NULL;

	backEnd.currentEntity = &tr.worldEntity;

    Bepc_UpdateNumDrawSurfs(numDrawSurfs);


    uint32_t i;
	for (i = 0; i < numDrawSurfs; ++i)
    {
		
        if ( pSurf[i].sort == oldSort )
        {
			// fast path, same as previous sort
			rb_surfaceTable[ *pSurf[i].surType ]( pSurf[i].surType );
			continue;
		}

		oldSort = pSurf[i].sort;
		

        int entityNum, fogNum, dlighted;
        shader_t* shader = NULL;

        R_DecomposeSort( oldSort, &entityNum, &shader, &fogNum, &dlighted );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( (shader != oldShader) || 
             (fogNum != oldFogNum) || 
             (dlighted != oldDlighted) || 
             (entityNum != oldEntityNum && !shader->entityMergable) )
        {
			if ( (oldShader != NULL) && (tess.numVertexes != 0))
            {
				RB_EndSurface(&tess);
			}
			RB_BeginSurface( shader, fogNum, &tess );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum )
        {
            oldEntityNum = entityNum;
			
            if ( entityNum != REFENTITYNUM_WORLD )
            {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
                // avoid a read after write 
                float temp = originalTime - backEnd.currentEntity->e.shaderTime;
                R_SetRefFloatTime( temp );

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				
                tess.shaderTime = temp - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );


				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
				}
			}
            else
            {
				backEnd.currentEntity = &tr.worldEntity;
				
                R_SetRefFloatTime( originalTime );
				
                backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = originalTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}

			// VULKAN
            set_modelview_matrix(backEnd.or.modelMatrix);
        }

		// add the triangles for this surface
		// rb_surfaceTable[ *pSurf->surType ]( pSurf->surType );
		rb_surfaceTable[ *pSurf[i].surType ]( pSurf[i].surType );
	}

	// backEnd.refdef.floatTime = originalTime;
    R_SetRefFloatTime( originalTime );
	
    // draw the contents of the last shader batch
	if ( (oldShader != NULL) && (tess.numVertexes != 0))
    {
		RB_EndSurface(&tess);
	}

	// go back to the world modelview matrix
    set_modelview_matrix(backEnd.viewParms.world.modelMatrix);
}

