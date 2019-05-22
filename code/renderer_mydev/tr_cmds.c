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

volatile renderCommandList_t	*renderCommandList;



void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		memset( &tr.pc, 0, sizeof( tr.pc ) );
		memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		ri.Printf (PRINT_ALL, "%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3, 
			R_SumOfUsedImages()/(1000000.0f)); 
	} else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
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

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}


void R_IssueRenderCommands( qboolean runPerformanceCounters )
{
	renderCommandList_t	*cmdList = &backEndData[0]->commands;
	assert(cmdList); // bk001205
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;


	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if( runPerformanceCounters ) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer )
    {
		// let it start on the new batch
		RB_ExecuteRenderCommands( cmdList->cmds );
	}
}

/*
====================
Issue any pending commands and wait for them to complete.
====================
*/
void R_IssuePendingRenderCommands( void )
{
	if ( !tr.registered )
	{
		return;
	}
	R_IssueRenderCommands( qfalse );
}


/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void *R_GetCommandBuffer( int bytes )
{
	renderCommandList_t	*cmdList = &backEndData[0]->commands;

	// always leave room for the end of list command
	if ( cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS ) {
		if ( bytes > MAX_RENDER_COMMANDS - 4 ) {
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
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
	if ( !tr.registered ) {
		return;
	}

	tr.frameCount++;

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified )
    {
		R_IssueRenderCommands( qfalse );
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if(r_gamma->modified)
    {
		r_gamma->modified = qfalse;

		R_IssueRenderCommands( qfalse );
		R_SetColorMappings();
	}

    // check for errors
    if ( !r_ignoreGLErrors->integer ) {
        int	err;

		R_IssueRenderCommands( qfalse );
        if ( ( err = qglGetError() ) != GL_NO_ERROR ) {
            ri.Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!\n", err );
        }
    }

	// draw buffer stuff
	drawBufferCommand_t	* cmd = (drawBufferCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;

    if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) )
    {
        cmd->buffer = (int)GL_FRONT;
    }
	else
	{
        cmd->buffer = (int)GL_BACK;
    }

}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	swapBuffersCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (swapBuffersCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

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

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t	*cmd = (const videoFrameCommand_t *)data;
/*
	byte				*cBuf;
	size_t				memcount, linelen;
	int				padwidth, avipadwidth, padlen, avipadlen;
	GLint packAlign;

	cmd = (const videoFrameCommand_t *)data;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen = padwidth - linelen;
	// AVI line padding
	avipadwidth = PAD(linelen, 4);
	avipadlen = avipadwidth - linelen;

	cBuf = PADP(cmd->captureBuffer, packAlign);

	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
	              GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg) {
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
		                        90, cmd->width, cmd->height, cBuf, padlen);
		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else {
		byte *lineend, *memend;
		byte *srcptr, *destptr;

		srcptr = cBuf;
		destptr = cmd->encodeBuffer;
		memend = srcptr + memcount;

		// swap R and B and remove line paddings
		while(srcptr < memend) {
			lineend = srcptr + linelen;
			while(srcptr < lineend) {
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}

			memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}
*/
	return (const void *)(cmd + 1);
}


