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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"
#include "tr_globals.h"

#include "tr_cvar.h"
#include "../renderercommon/ref_import.h"


extern void R_ToggleSmpFrame(void);



/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void* R_GetCommandBuffer( int bytes )
{
	renderCommandList_t	*cmdList = &backEndData->commands;

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


/*
=============
submits a single 'draw' command into the command queue
=============
*/
void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	drawSurfsCommand_t* cmd = (drawSurfsCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
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
    if ( !tr.registered ) {
        return;
    }
    
    setColorCommand_t* cmd = (setColorCommand_t*) R_GetCommandBuffer(sizeof(setColorCommand_t));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	
    if(rgba)
    {
        cmd->color[0] = rgba[0];
        cmd->color[1] = rgba[1];
        cmd->color[2] = rgba[2];
        cmd->color[3] = rgba[3];
    }
    else
    {
        // color white
        cmd->color[0] = 1.0f;
        cmd->color[1] = 1.0f;
        cmd->color[2] = 1.0f;
        cmd->color[3] = 1.0f;
    }
}


void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader )
{
    if (!tr.registered) {
        return;
    }
    stretchPicCommand_t* cmd = (stretchPicCommand_t*) R_GetCommandBuffer(sizeof(stretchPicCommand_t));
	if ( !cmd ) {
		return;
	}
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


/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/

void RE_BeginFrame( void )
{

	if ( !tr.registered ) {
		return;
	}

	tr.frameCount++;

	//
	// draw buffer stuff
	//
	drawBufferCommand_t* cmd = (drawBufferCommand_t*) R_GetCommandBuffer(sizeof(drawBufferCommand_t));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;

}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec )
{

	if ( !tr.registered ) {
		return;
	}
	swapBuffersCommand_t* cmd = (swapBuffersCommand_t*) R_GetCommandBuffer(sizeof(swapBuffersCommand_t));
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

