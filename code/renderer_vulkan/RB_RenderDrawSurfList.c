#include "tr_globals.h" //tr.worldEn
#include "tr_backend.h"
#include "vk_shade_geometry.h"
#include "tr_surface.h"
#include "tr_light.h"
#include "R_ShaderCommands.h"   //tess
#include "tr_shadows.h"
#include "R_RotateForViewer.h"

#include "FixRenderCommandList.h"

void RB_RenderDrawSurfList(const drawSurf_t* const pDrawSurfs, int numDrawSurfs, const trRefdef_t * const pRefdef, viewParms_t * pViewPar )
{

    backEnd.refdef = *pRefdef;
    backEnd.viewParms = *pViewPar;


	// save original time for entity shader offsets
	float originalTime = backEnd.refdef.floatTime;

    // Any mirrored or portaled views have already been drawn, 
    // so prepare to actually render the visible surfaces for this view
	// clear the z buffer, set the modelview, etc
	// RB_BeginDrawingView ();

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;


	// ensures that depth writes are enabled for the depth clear
    
	// VULKAN
    vk_clearDepthStencilAttachments();


  
    if ( backEnd.refdef.rd.rdflags & RDF_HYPERSPACE )
	{
		//RB_Hyperspace();
        // A player has predicted a teleport, but hasn't arrived yet
        const float c = ( backEnd.refdef.rd.time & 255 ) / 255.0f;
        const float color[4] = { c, c, c, 1 };

        // so short, do we really need this?
	    vk_clearColorAttachments(color);

	    backEnd.isHyperspace = qtrue;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}


	// draw everything

	int oldEntityNum = -1;
	int oldFogNum = -1;
	int oldDlighted = qfalse;
	unsigned int oldSort = -1;
    


    shader_t* oldShader = NULL;

	backEnd.currentEntity = &tr.worldEntity;
	backEnd.pc.c_surfaces += numDrawSurfs;



    int	i;
	const drawSurf_t* pSurf = pDrawSurfs;
	for (i = 0; i < numDrawSurfs; ++i, ++pSurf)
    {
		
        if ( pSurf->sort == oldSort )
        {
			// fast path, same as previous sort
			rb_surfaceTable[ *pSurf->surface ]( pSurf->surface );
			continue;
		}

		oldSort = pSurf->sort;
		

        int entityNum, fogNum, dlighted;
        shader_t* shader;

        R_DecomposeSort( pSurf->sort, &entityNum, &shader, &fogNum, &dlighted );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( (shader != oldShader) || 
             (fogNum != oldFogNum) || 
             (dlighted != oldDlighted) || 
             (entityNum != oldEntityNum && !shader->entityMergable) )
        {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum )
        {
			if ( entityNum != REFENTITYNUM_WORLD )
            {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

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
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}


			// VULKAN
            set_modelview_matrix(backEnd.or.modelMatrix);
            oldEntityNum = entityNum;
        }

		// add the triangles for this surface
		rb_surfaceTable[ *pSurf->surface ]( pSurf->surface );

	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	// go back to the world modelview matrix
    set_modelview_matrix(backEnd.viewParms.world.modelMatrix);


	// darken down any stencil shadows
	RB_ShadowFinish();
}

