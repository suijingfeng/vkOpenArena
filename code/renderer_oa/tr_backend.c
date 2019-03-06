/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"



extern void (APIENTRYP qglActiveTextureARB) (GLenum texture);
extern void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);


extern shaderCommands_t tess;
extern cvar_t* r_flares;

backEndState_t backEnd;
backEndData_t* backEndData;


static const float s_flipMatrix[16] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};




/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void )
{
	float c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}



/*
 * RB_BeginDrawingView: Any mirrored or portaled views have already been drawn, 
 * so prepare to actually render the visible surfaces for this view
 */
static void RB_BeginDrawingView(void)
{
	int clearBits = 0;

	// sync with gl if needed
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}
    else if (!glState.finishCalled)
    {
        qglFinish();
		glState.finishCalled = qtrue;
    }

	// we will need to change the projection matrix before drawing 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
	qglMatrixMode(GL_MODELVIEW);

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );


	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
		qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f );	// FIXME: get color of sky
#else
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
#endif
	}
	qglClear( clearBits );

	if( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal )
	{
		float	plane[4];
		GLdouble	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.or.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.or.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.or.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.or.origin) - plane[3];

		qglLoadMatrixf( s_flipMatrix );
		qglClipPlane (GL_CLIP_PLANE0, plane2);
		qglEnable (GL_CLIP_PLANE0);
	}
    else
    {
		qglDisable (GL_CLIP_PLANE0);
	}
}



static void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	shader_t		*shader, *oldShader = NULL;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	qboolean		isCrosshair;
	int				i;
	drawSurf_t		*drawSurf;
	double			originalTime;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	// draw everything
	int oldEntityNum = -1;
    int oldFogNum = -1;
	int oldSort = -1;
    
	qboolean depthRange = qfalse;
    qboolean oldDepthRange = qfalse;
	qboolean wasCrosshair = qfalse;
	qboolean oldDlighted = qfalse;

	backEnd.currentEntity = &tr.worldEntity;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs; i < numDrawSurfs ; i++, drawSurf++)
    {
		if ( drawSurf->sort == oldSort )
        {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( (shader != NULL) && ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted 
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) )
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
		if( entityNum != oldEntityNum )
        {
			depthRange = isCrosshair = qfalse;

			if ( entityNum != REFENTITYNUM_WORLD )
            {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];

				// FIXME: e.shaderTime must be passed as int to avoid fp-precision loss issues
				backEnd.refdef.floatTime = originalTime - (double)backEnd.currentEntity->e.shaderTime;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights )
                {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
					
					if(backEnd.currentEntity->e.renderfx & RF_CROSSHAIR)
						isCrosshair = qtrue;
				}
			}
			else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}

			qglLoadMatrixf( backEnd.or.modelMatrix );

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//
			if (oldDepthRange != depthRange || wasCrosshair != isCrosshair)
			{
				if (depthRange)
				{
					if(!oldDepthRange)
						qglDepthRange (0, 0.3);
				}
				else
				{
					qglDepthRange (0, 1);
				}

				oldDepthRange = depthRange;
				wasCrosshair = isCrosshair;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL)
    {
		RB_EndSurface();
	}

	// go back to the world modelview matrix
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange )
    {
		qglDepthRange (0, 1);
	}


	// darken down any stencil shadows
	RB_ShadowFinish();		

	// add light flares on lights that aren't obscured
	if( r_flares->integer )
		RB_RenderFlares();

	if (r_drawSun->integer)
	{
		RB_DrawSun();
	}
}


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/


static void RB_SetGL2D (void)
{
	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA |
				GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}


static const void* RB_SetColor( const void *data )
{
	const setColorCommand_t	*cmd = (const setColorCommand_t *)data;
	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;
	return (const void *)(cmd + 1);
}


static const void *RB_StretchPic( const void *data )
{
	const stretchPicCommand_t* cmd = (const stretchPicCommand_t *)data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader_t * shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	//RB_CHECKOVERFLOW( 4, 6 );
    if ( (tess.numVertexes + 4 >= SHADER_MAX_VERTEXES) || (tess.numIndexes + 6 >= SHADER_MAX_INDEXES) )
    {
        RB_CheckOverflow(4,6);
    }

	const unsigned int n0 = tess.numVertexes;
	const unsigned int n1 = n0 + 1;
	const unsigned int n2 = n0 + 2;
	const unsigned int n3 = n0 + 3;

	{
		unsigned int numIndexes = tess.numIndexes;

		tess.indexes[ numIndexes ] = n3;
		tess.indexes[ numIndexes + 1 ] = n0;
		tess.indexes[ numIndexes + 2 ] = n2;
		tess.indexes[ numIndexes + 3 ] = n2;
		tess.indexes[ numIndexes + 4 ] = n0;
		tess.indexes[ numIndexes + 5 ] = n1;
	}

	{
		const unsigned char r = backEnd.color2D[0];
		const unsigned char g = backEnd.color2D[1];
		const unsigned char b = backEnd.color2D[2];
		const unsigned char a = backEnd.color2D[3];

		tess.vertexColors[ n0 ][ 0 ] = r;
		tess.vertexColors[ n0 ][ 1 ] = g;
		tess.vertexColors[ n0 ][ 2 ] = b;
		tess.vertexColors[ n0 ][ 3 ] = a;

		tess.vertexColors[ n1 ][ 0 ] = r;
		tess.vertexColors[ n1 ][ 1 ] = g;
		tess.vertexColors[ n1 ][ 2 ] = b;
		tess.vertexColors[ n1 ][ 3 ] = a;

		tess.vertexColors[ n2 ][ 0 ] = r;
		tess.vertexColors[ n2 ][ 1 ] = g;
		tess.vertexColors[ n2 ][ 2 ] = b;
		tess.vertexColors[ n2 ][ 3 ] = a;

		tess.vertexColors[ n3 ][ 0 ] = r;
		tess.vertexColors[ n3 ][ 1 ] = g;
		tess.vertexColors[ n3 ][ 2 ] = b;
		tess.vertexColors[ n3 ][ 3 ] = a;
	}


	{
		tess.xyz[ n0 ][0] = cmd->x;
		tess.xyz[ n0 ][1] = cmd->y;
		tess.xyz[ n0 ][2] = 0;

		tess.xyz[ n1 ][0] = cmd->x + cmd->w;
		tess.xyz[ n1 ][1] = cmd->y;
		tess.xyz[ n1 ][2] = 0;

		tess.xyz[ n2 ][0] = cmd->x + cmd->w;
		tess.xyz[ n2 ][1] = cmd->y + cmd->h;
		tess.xyz[ n2 ][2] = 0;

		tess.xyz[ n3 ][0] = cmd->x;
		tess.xyz[ n3 ][1] = cmd->y + cmd->h;
		tess.xyz[ n3 ][2] = 0;
	}

	{
		tess.texCoords[ n0 ][0][0] = cmd->s1;
		tess.texCoords[ n0 ][0][1] = cmd->t1;

		tess.texCoords[ n1 ][0][0] = cmd->s2;
		tess.texCoords[ n1 ][0][1] = cmd->t1;

		tess.texCoords[ n2 ][0][0] = cmd->s2;
		tess.texCoords[ n2 ][0][1] = cmd->t2;

		tess.texCoords[ n3 ][0][0] = cmd->s1;
		tess.texCoords[ n3 ][0][1] = cmd->t2;
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	return (const void *)(cmd + 1);
}



static const void *RB_DrawSurfs(const void *data)
{
	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	const drawSurfsCommand_t* cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	//TODO Maybe check for rdf_noworld stuff but q3mme has full 3d ui
	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

	return (const void *)(cmd + 1);
}



static const void* RB_DrawBuffer(const void *data)
{
	const drawBufferCommand_t* cmd = (const drawBufferCommand_t *)data;

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer )
    {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)(cmd + 1);
}



static const void *RB_ColorMask(const void *data)
{
    typedef struct
    {
        int commandId;
        GLboolean rgba[4];
    } colorMaskCommand_t;
    
	const colorMaskCommand_t *cmd = data;
	
	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}


static const void *RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = data;
	
	if(tess.numIndexes)
		RB_EndSurface();

	// texture swapping test
	if (r_showImages->integer)
		RB_ShowImages();

	qglClear(GL_DEPTH_BUFFER_BIT);
	
	return (const void *)(cmd + 1);
}


static const void* RB_SwapBuffers( const void *data )
{
	// finish any 2D drawing if needed
	if ( tess.numIndexes )
    {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer )
    {
		RB_ShowImages();
	}


	const swapBuffersCommand_t *cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer )
    {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}


	if ( !glState.finishCalled ) {
		qglFinish();
	}

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;


	return (const void *)(cmd + 1);
}


/*
============
R_GetCommandBuffer: make sure there is enough command space
============
*/
static void *R_GetCommandBuffer(int bytes)
{
	renderCommandList_t	*cmdList = &backEndData->commands;
	
    bytes = (bytes+sizeof(void *)-1) & ~(sizeof(void *)-1);

	// always leave room for the end of list command
	if ( cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS )
    {
		if ( bytes > MAX_RENDER_COMMANDS - 4 ) {
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

static void RB_ExecuteRenderCommands(const void *data)
{
	int	t1 = ri.Milliseconds();
	while( 1 )
    {
		data = PADP(data, sizeof(void *));

		switch( *(const int *)data )
		{
            case RC_SET_COLOR:
                data = RB_SetColor( data ); break;
            case RC_STRETCH_PIC:
                data = RB_StretchPic( data ); break;
            case RC_DRAW_SURFS:
                data = RB_DrawSurfs( data ); break;
            case RC_DRAW_BUFFER:
                data = RB_DrawBuffer( data ); break;
            case RC_SWAP_BUFFERS:
                data = RB_SwapBuffers( data ); break;
            case RC_SCREENSHOT:
                data = RB_TakeScreenshotCmd( data ); break;
            case RC_VIDEOFRAME:
                data = RB_TakeVideoFrameCmd( data ); break;
            case RC_COLORMASK:
                data = RB_ColorMask(data); break;
            case RC_CLEARDEPTH:
                data = RB_ClearDepth(data); break;
            case RC_END_OF_LIST:
			default:	// stop rendering
				backEnd.pc.msec = ri.Milliseconds() - t1;
			return;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////

void GL_Bind(image_t *image)
{
	int texnum;

	if ( !image )
    {
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	}
    else
    {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage )
    {		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum )
    {
		if ( image )
			image->frameUsed = tr.frameCount;
	
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture(GL_TEXTURE_2D, texnum);
	}
}


void GL_SelectTexture(int unit)
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if ( unit == 0 )
	{
		qglActiveTextureARB( GL_TEXTURE0_ARB );
		qglClientActiveTextureARB( GL_TEXTURE0_ARB );
	}
	else if ( unit == 1 )
	{
		qglActiveTextureARB( GL_TEXTURE1_ARB );
		qglClientActiveTextureARB( GL_TEXTURE1_ARB );
	}
	else if ( unit == 2 )
	{
		qglActiveTextureARB( GL_TEXTURE2_ARB );
		qglClientActiveTextureARB( GL_TEXTURE2_ARB );
	}
	else if ( unit == 3 )
	{
		qglActiveTextureARB( GL_TEXTURE3_ARB );
		qglClientActiveTextureARB( GL_TEXTURE3_ARB );
	}
    else
    {
		ri.Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}

	glState.currenttmu = unit;
}


void GL_Cull( int cullType )
{
	if( glState.faceCulling == cullType )
    {
		return;
	}
	
	glState.faceCulling = cullType;
	if( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qboolean cullFront;
		qglEnable( GL_CULL_FACE );
		cullFront = (cullType == CT_FRONT_SIDED);
		if ( backEnd.viewParms.isMirror )
		{
			cullFront = !cullFront;
		}
		qglCullFace( cullFront ? GL_FRONT : GL_BACK );
	}
}

/*
static void GL_BindMultitexture( image_t *image0, GLuint env0, image_t *image1, GLuint env1 )
{
	int	texnum0 = image0->texnum;
    int texnum1 = image1->texnum;

	if( r_nobind->integer && tr.dlightImage )
    {		// performance evaluation option
		texnum0 = texnum1 = tr.dlightImage->texnum;
	}

	if( glState.currenttextures[1] != texnum1 )
    {
		GL_SelectTexture( 1 );
		image1->frameUsed = tr.frameCount;
		glState.currenttextures[1] = texnum1;
		qglBindTexture( GL_TEXTURE_2D, texnum1 );
	}
	if( glState.currenttextures[0] != texnum0 )
    {
		GL_SelectTexture( 0 );
		image0->frameUsed = tr.frameCount;
		glState.currenttextures[0] = texnum0;
		qglBindTexture( GL_TEXTURE_2D, texnum0 );
	}
}
*/

void GL_TexEnv( int env )
{
	if ( env == glState.texEnv[glState.currenttmu] )
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch ( env )
	{
        case GL_MODULATE:
            qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
            break;
        case GL_REPLACE:
            qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
            break;
        case GL_DECAL:
            qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
            break;
        case GL_ADD:
            qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
            break;
        default:
            ri.Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
            break;
	}
}


/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state in Q3.
*/
void GL_State( unsigned long stateBits )
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_EQUAL )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
                case GLS_SRCBLEND_ZERO:
                    srcFactor = GL_ZERO;
                    break;
                case GLS_SRCBLEND_ONE:
                    srcFactor = GL_ONE;
                    break;
                case GLS_SRCBLEND_DST_COLOR:
                    srcFactor = GL_DST_COLOR;
                    break;
                case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
                    srcFactor = GL_ONE_MINUS_DST_COLOR;
                    break;
                case GLS_SRCBLEND_SRC_ALPHA:
                    srcFactor = GL_SRC_ALPHA;
                    break;
                case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
                    srcFactor = GL_ONE_MINUS_SRC_ALPHA;
                    break;
                case GLS_SRCBLEND_DST_ALPHA:
                    srcFactor = GL_DST_ALPHA;
                    break;
                case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
                    srcFactor = GL_ONE_MINUS_DST_ALPHA;
                    break;
                case GLS_SRCBLEND_ALPHA_SATURATE:
                    srcFactor = GL_SRC_ALPHA_SATURATE;
                    break;
                default:
                    ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
                    break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
                case GLS_DSTBLEND_ZERO:
                    dstFactor = GL_ZERO;
                    break;
                case GLS_DSTBLEND_ONE:
                    dstFactor = GL_ONE;
                    break;
                case GLS_DSTBLEND_SRC_COLOR:
                    dstFactor = GL_SRC_COLOR;
                    break;
                case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
                    dstFactor = GL_ONE_MINUS_SRC_COLOR;
                    break;
                case GLS_DSTBLEND_SRC_ALPHA:
                    dstFactor = GL_SRC_ALPHA;
                    break;
                case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
                    dstFactor = GL_ONE_MINUS_SRC_ALPHA;
                    break;
                case GLS_DSTBLEND_DST_ALPHA:
                    dstFactor = GL_DST_ALPHA;
                    break;
                case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
                    dstFactor = GL_ONE_MINUS_DST_ALPHA;
                    break;
                default:
                    ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
                    break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
            case 0:
                qglDisable( GL_ALPHA_TEST );
                break;
            case GLS_ATEST_GT_0:
                qglEnable( GL_ALPHA_TEST );
                qglAlphaFunc( GL_GREATER, 0.0f );
                break;
            case GLS_ATEST_LT_80:
                qglEnable( GL_ALPHA_TEST );
                qglAlphaFunc( GL_LESS, 0.5f );
                break;
            case GLS_ATEST_GE_80:
                qglEnable( GL_ALPHA_TEST );
                qglAlphaFunc( GL_GEQUAL, 0.5f );
                break;
            default:
                assert( 0 );
                break;
		}
	}

	glState.glStateBits = stateBits;
}

/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const unsigned char *data, int client, qboolean dirty)
{
	int	start, end;

	if ( !tr.registered ) {
		return;
	}
	R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definitely want to sync every frame for the cinematics
	qglFinish();

	start = 0;
	if ( r_speeds->integer ) {
		start = ri.Milliseconds();
	}

    {
        // make sure rows and cols are powers of 2
        int	i, j;

        for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
        }
        for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
        }

        
        if ( (( 1 << i ) != cols) || (( 1 << j ) != rows))
        {
            ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i",
                    cols, rows);
        }
    }

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( (cols != tr.scratchImage[client]->width) || 
            (rows != tr.scratchImage[client]->height) )
    {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	
	}
    else
    {
		if (dirty)
        {
			// otherwise, just subimage upload it so that 
            // drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( r_speeds->integer ) {
		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	RB_SetGL2D();

	qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

	qglBegin (GL_QUADS);
	qglTexCoord2f ( 0.5f / cols,  0.5f / rows );
	qglVertex2f (x, y);
	qglTexCoord2f ( ( cols - 0.5f ) / cols ,  0.5f / rows );
	qglVertex2f (x+w, y);
	qglTexCoord2f ( ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows );
	qglVertex2f (x+w, y+h);
	qglTexCoord2f ( 0.5f / cols, ( rows - 0.5f ) / rows );
	qglVertex2f (x, y+h);
	qglEnd ();
}


void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}



/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever was there.
This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/

void RB_ShowImages(void)
{
	int	i;


	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	int start = ri.Milliseconds();

	for( i=0 ; i<tr.numImages ; i++ )
    {
		image_t	* image = tr.images[i];

		float w = glConfig.vidWidth / 20;
		float h = glConfig.vidHeight / 15;
		float x = i % 20 * w;
		float y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind( image );
		qglBegin (GL_QUADS);
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	int end = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );
}




//////////////////////////////// merged from tr_cmds.c /////////////////////////

static void R_PerformanceCounters(void)
{
	if ( !r_speeds->integer )
    {
		// clear the counters even if we aren't printing
		memset( &tr.pc, 0, sizeof( tr.pc ) );
		memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1)
    {
		ri.Printf (PRINT_ALL, "%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3, 
			R_SumOfUsedImages()/(1000000.0f), backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight) ); 
	}
    else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	}
    else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	}
    else if (r_speeds->integer == 4)
    {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	} 
	else if (r_speeds->integer == 5 )
	{
		ri.Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		ri.Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}


static void R_IssueRenderCommands(qboolean runPerformanceCounters)
{
	renderCommandList_t	*cmdList = &backEndData->commands;
	assert(cmdList);
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if( runPerformanceCounters )
		R_PerformanceCounters();


	// actually start the commands going
	if ( !r_skipBackEnd->integer )
    {
		// let it start on the new batch
		RB_ExecuteRenderCommands( cmdList->cmds );
	}
}


//////////////////////////////////////////////////////////////////////////

/*
====================
R_IssuePendingRenderCommands: Issue any pending commands and wait for them to complete.
====================
*/
void R_IssuePendingRenderCommands( void )
{
	if ( !tr.registered ) 
		return;
	
	renderCommandList_t	*cmdList = &backEndData->commands;
	assert(cmdList);
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	// actually start the commands going
	if ( !r_skipBackEnd->integer )
    {
		// let it start on the new batch
		RB_ExecuteRenderCommands( cmdList->cmds );
	}
}




void R_AddDrawSurfCmd(drawSurf_t *drawSurfs, int numDrawSurfs )
{
	drawSurfsCommand_t *cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}


/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void RE_SetColor( const float *rgba )
{
	setColorCommand_t *cmd = R_GetCommandBuffer( sizeof(*cmd) );
    if( !cmd ) {
		return;
	}

    if( !tr.registered )
    {
        return;
    }

	cmd->commandId = RC_SET_COLOR;
	if ( !rgba )
    {
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}


void RE_StretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{    
    if (tr.registered)
	{

		stretchPicCommand_t* cmd = R_GetCommandBuffer( sizeof( *cmd ) );
		if ( cmd != NULL )
		{
			cmd->commandId = RC_STRETCH_PIC;
			cmd->shader = R_GetShaderByHandle( hShader );
			cmd->x = x;
			cmd->y = y;
			cmd->w = w;
			cmd->h = h;
			cmd->s1 = s1;
			cmd->t1 = t1;
			cmd->s2 = s2;
			cmd->t2 = t2;
		}
	}
}



void RE_BeginFrame(void)
{
	if ( !tr.registered )
		return;

	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// do overdraw measurement
	//
	if( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else if ( r_shadows->integer == 2 )
		{
			ri.Printf( PRINT_ALL, "Warning: stencil shadows and overdraw measurement are mutually exclusive\n" );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else
		{
			R_IssuePendingRenderCommands();
			qglEnable( GL_STENCIL_TEST );
			qglStencilMask( ~0U );
			qglClearStencil( 0U );
			qglStencilFunc( GL_ALWAYS, 0U, ~0U );
			qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified )
        {
			R_IssuePendingRenderCommands();
			qglDisable( GL_STENCIL_TEST );
		}
		r_measureOverdraw->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if(r_gamma->modified)
    {
		r_gamma->modified = qfalse;

		R_IssuePendingRenderCommands();
		R_SetColorMappings();
	}

	// check for errors
	if ( !r_ignoreGLErrors->integer )
	{
		int	err;

		R_IssuePendingRenderCommands();
		if ((err = qglGetError()) != GL_NO_ERROR)
			ri.Error(ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!", err);
	}

}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec )
{
	swapBuffersCommand_t *cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
		return;
    if ( !tr.registered )
		return;

	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

	R_InitNextFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}


void RE_TakeVideoFrame( int width, int height, unsigned char *captureBuffer, unsigned char *encodeBuffer, qboolean motionJpeg )
{
	videoFrameCommand_t	*cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if( !cmd )
		return;

	if( !tr.registered )
		return;


	cmd->commandId = RC_VIDEOFRAME;

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}

void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg )
{
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}


