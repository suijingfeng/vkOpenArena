#ifndef TR_CMDS_H_
#define TR_CMDS_H_

#include "trRefDef.h"
#include "viewParms.h"
/*
=========================================================

RENDERER BACK END COMMAND QUEUE

=========================================================
*/

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct {
	unsigned char cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct {
	int		commandId;
} drawBufferCommand_t;


typedef struct {
	int		commandId;
} swapBuffersCommand_t;

typedef struct {
	int		commandId;
} endFrameCommand_t;

typedef struct {
	int		commandId;
	struct shader_s* shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	struct drawSurf_s * drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;


typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
} renderCommand_t;

typedef struct {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	qboolean jpeg;
} screenshotCommand_t;


/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/


void* R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_IssueRenderCommands( qboolean runPerformanceCounters );

void R_AddDrawSurfCmd( struct drawSurf_s * const drawSurfs, uint32_t numDrawSurfs );

#endif
