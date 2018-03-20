/*
===========================================================================
Copyright (C) 2006 Sjoerd van der Berg ( harekiet @ gmail.com )

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
// fx_local.h -- private fx definitions

#define FX_HASH_SHIFT 8
#define FX_HASH_SIZE ( 1 << FX_HASH_SHIFT )
#define FX_HASH_MASK (FX_HASH_SIZE - 1)

#define FX_ACTIVE_HASH_SHIFT 5
#define FX_ACTIVE_HASH_SIZE ( 1 << FX_ACTIVE_HASH_SHIFT )
#define FX_ACTIVE_HASH_MASK (FX_ACTIVE_HASH_SIZE - 1)

#define FX_MAX_SCRIPTS 4096

#define FX_MAX_ENTITIES (16*1024)

#define FX_EMIT_TRACE		0x001
#define FX_EMIT_SINK		0x002
#define FX_EMIT_MOVE		0x004
#define FX_EMIT_PARENT		0x010
#define FX_EMIT_IMPACT		0x020

#define FX_EMIT_MASKSIZE	16				//Probably enough space

#define FX_ENT_STATIONARY	0x001	
#define FX_ENT_DEAD			0x002

#define FX_SCRIPT_PARENT	0x001
#define FX_STACK_SIZE		128
#define FX_VECTOR_PARENT	0x10000

#define FX_MASK_PARENT		0x00001
#define FX_MASK_ORIGIN		0x00002
#define FX_MASK_VELOCITY	0x00004
#define FX_MASK_SHADER		0x00008
#define FX_MASK_SIZE		0x00010
#define FX_MASK_ROTATE		0x00020
#define FX_MASK_COLOR		0x00040

#define FX_MASK_DIR			0x00080
#define FX_MASK_MODEL		0x00100
#define FX_MASK_AXIS		0x00200
#define FX_MASK_ANGLES		0x00400
#define FX_MASK_WIDTH		0x00800
#define FX_MASK_T0			0x01000
#define FX_MASK_T1			0x02000
#define FX_MASK_T2			0x04000
#define FX_MASK_T3			0x08000
#define FX_MASK_V0			0x10000
#define FX_MASK_V1			0x20000
#define FX_MASK_V2			0x40000
#define FX_MASK_V3			0x80000



struct fxHashEntry_s;
struct fxEntity_s;
struct fxInitScript_s;

typedef struct fxAtive_s {
	struct fxAtive_s	*next;
	unsigned int		key1, key2;
	int					beenUsed;
} fxActive_t;

//Making any changes to this order you'll also need to modify the mask/unpacking stuff
typedef struct {
	vec3_t			origin;
	vec3_t			velocity;

	float			size, rotate;
	color4ub_t		color;
	qhandle_t		shader;

	vec3_t			dir;
	float			width;
	vec3_t			angles;
	qhandle_t		model;
	vec3_t			axis[3];

	float			t0, t1, t2, t3;
	vec3_t			v0, v1, v2, v3;
	const fxParent_t	*parent;

	float			lerp;
	float			loop;
	float			life;

	struct fxEntity_s	*entity;
	int				shaderTime;
	unsigned		int key;

	int				stackUsed;
	float			stackData[FX_STACK_SIZE];
} fxRun_t;

const void *fxRunMath( fxRun_t *run, const void *data, float *value );
typedef const void *(*fxHandler_t)( fxRun_t *run, const void * );

static ID_INLINE const float *fxFloatSrc( const fxRun_t *run, int offset ) {
	if ( offset & FX_VECTOR_PARENT)
		return (float *) (((char *)run->parent) + (offset & ~FX_VECTOR_PARENT) );
	else
		return	(float *) (((char *)run) + offset );
}

static ID_INLINE float *fxFloatDst( const fxRun_t *run, int offset ) {
	return	(float *) (((char *)run) + offset );
}

typedef enum {
	fxCmdHalt,
	fxCmdSkip,

	fxCmdEmitter,
	fxCmdKill,
	fxCmdRepeat,
	fxCmdOnce,
	fxCmdIf,
	fxCmdInterval,
	fxCmdDistance,


	fxCmdSprite,
	fxCmdBeam,
	fxCmdLight,
	fxCmdDirModel,
	fxCmdAnglesModel,
	fxCmdAxisModel,
	fxCmdQuad,
	fxCmdRings,
	fxCmdSpark,
	fxCmdDecal,

	fxCmdTrace,
	fxCmdScript,

	fxCmdColor,
	fxCmdColorList,
	fxCmdColorBlend,
	fxCmdColorHue,
	fxCmdColorScale,
	fxCmdColorMath,
	fxCmdColorFade,
	fxCmdAlphaFade,

	fxCmdShader,
	fxCmdShaderList,
	fxCmdModel,
	fxCmdModelList,
	fxCmdSound,
	fxCmdSoundList,
	fxCmdLoopSound,
	fxCmdVibrate,

	fxCmdPush,
	fxCmdPop,
	fxCmdPushParent,

	fxCmdScale,
	fxCmdCopy,
	fxCmdAdd,
	fxCmdAddScale,
	fxCmdSub,
	fxCmdSubScale,
	fxCmdRotateAround,
	fxCmdInverse,
	fxCmdNormalize,
	fxCmdPerpendicular,
	fxCmdRandom,
	fxCmdClear,
	fxCmdWobble,
	fxCmdMakeAngles,
	fxCmdValue,
} fxCommand_t;

typedef struct  {
	int blockSize;
	void *blockData;
} fxRunBlock_t;

typedef struct fxScript_s {
	fxHandle_t				handle;
	struct fxHashEntry_s	*entry;
	struct fxScript_s		*remap;
	unsigned int			readMask;
	void					*data[0];
} fxScript_t;

typedef struct {
	int		renderfx;
} fxRunRender_t;

typedef struct {
	color4ub_t		value;
} fxRunColor_t;
typedef struct {
	unsigned int	count;
} fxRunColorBlend_t;
typedef struct {
	unsigned int	count;
} fxRunColorList_t;
typedef struct {
	float delay, scale;
} fxRunColorFade_t;
typedef struct {
	unsigned int	index;
} fxRunColorMath_t;
typedef struct {
	color4ub_t		value;
} fxRunAlpha_t;
typedef struct {
	int				size;
} fxRunAlphaScale_t;
typedef struct {
	float delay, scale;
} fxRunAlphaFade_t;
typedef struct {
	int				size;
} fxRunRed_t;
typedef struct {
	int				size;
} fxRunGreen_t;
typedef struct {
	int				size;
} fxRunBlue_t;

typedef struct {
	unsigned short	flags;
	unsigned short	size;
	unsigned short	emitRun;
	unsigned short	impactRun;
	unsigned short	deathRun;
	unsigned short	allocSize;
	byte			mask[FX_EMIT_MASKSIZE];
	float			impactSpeed;
	float			bounceFactor;
	float			sinkDelay, sinkDepth;
	float			gravity;
} fxRunEmitter_t;

typedef struct {
	int				src;
	int				dst;
} fxRunScale_t;
typedef struct {
	int				src;
	int				dst;
} fxRunWobble_t;
typedef struct {
	int				src;
	int				dst;
} fxRunCopy_t;
typedef struct {
	int				src;
	int				dst;
} fxRunMakeAngles_t;
typedef struct {
	int				dst;
} fxRunClear_t;
typedef struct {
	int				src;
	int				dst;
} fxRunInverse_t;
typedef struct {
	int				src;
	int				dst;
} fxRunNormalize_t;
typedef struct {
	int				src1;
	int				src2;
	int				dst;
} fxRunAdd_t;
typedef struct {
	int				src1;
	int				src2;
	int				dst;
} fxRunSub_t;
typedef struct {
	int				src;
	int				scale;
	int				dst;
} fxRunAddScale_t;
typedef struct {
	int				src;
	int				scale;
	int				dst;
} fxRunSubScale_t;
typedef struct {
	int				src;
	int				dir;
	int				dst;
} fxRunRotateAround_t;
typedef struct {
	int				src;
	int				dst;
} fxRunPerpendicular_t;
typedef struct {
	int				dst;
} fxRunRandom_t;

typedef struct {
	int				dst;
} fxRunValue_t;

typedef struct {
	qhandle_t		shader;
} fxRunShader_t;
typedef struct {
	unsigned int count;
} fxRunShaderList_t;

typedef struct {
	qhandle_t		model;
} fxRunModel_t;

typedef struct {
	unsigned int	count;
	qhandle_t		list[0];
} fxRunModelList_t;

typedef struct {
	int				flags;
	int				life;
} fxRunDecal_t;

typedef struct {
	void			*data;
} fxRunScript_t;

typedef struct {
	int				count;
	int				offset;
} fxRunPush_t;

typedef struct {
	int				count;
	int				offset;
} fxRunPushParent_t;

typedef struct {
	int				count;
	int				offset;
} fxRunPop_t;

typedef struct {
	int				size;
} fxRunRepeat_t;

typedef struct {
	int				size;
} fxRunOnce_t;
typedef struct {
	int				beenUsed;
} fxActiveOnce_t;


typedef struct {
	int				size;
} fxRunInterval_t;
typedef struct {
	int				beenUsed;
	int				nextTime;			
} fxActiveInterval_t;

typedef struct {
	int				size;
} fxRunSkip_t;
typedef struct {
	int				testCount;
	int				elseStep;
	int				size;
} fxRunIf_t;
typedef struct {
	int				size;
} fxRunDistance_t;
typedef struct {
	int				beenUsed;
	vec3_t			lastOrigin;
	float			distance;
} fxActiveDistance_t;

typedef struct {
	float			strength;
} fxRunVibrate_t;

typedef struct {
	unsigned int	count;
	sfxHandle_t		handle[0];
} fxRunSoundList_t;

typedef struct {
	sfxHandle_t		handle;
} fxRunSound_t;

typedef struct {
	sfxHandle_t		handle;
} fxRunLoopSound_t;

typedef struct fxEntity_s {
	struct		fxEntity_s *next;
	const		fxRunEmitter_t *emitter;

	int			flags;
	int			startTime;
	float		traceTime;
	float		moveTime, lifeScale;

	vec3_t		origin, velocity;
} fxEntity_t;

/* Just gonna stick with using next for now */
typedef struct fxHashEntry_s {
	const char *name;
	const char *text;
	fxScript_t *script;
	struct fxHashEntry_s *next;
} fxHashEntry_t;

#define FX_VIBRATE_QUEUE 32
typedef struct {
	fxScript_t			*scripts[FX_MAX_SCRIPTS];
	fxHashEntry_t		*entryHash[FX_HASH_SIZE];
	fxActive_t			*activeHash[FX_ACTIVE_HASH_SIZE];

	int					scriptCount;
	int					*allocSearch;
	int					*allocStart;
	int					allocSize;
	int					time, oldTime;
	float				timeFraction;
	float				deltaTime;
	int					seed;
	fxEntity_t			*entityActive;
	fxEntity_t			*entityNew;
	struct {
		int				entitySize, activeSize;
		int				entityCount, activeCount;
	} last;
	struct {
		int used, time;
		float offset, magnitude;
		struct {
			vec3_t	origin;
			float	strength;
		} queue[FX_VIBRATE_QUEUE];
	} vibrate;
	struct {
		fxHashEntry_t *entries;			
	} alloc;
} fxMain_t;

void fxInitParser( void );
void fxFreeMemory( void );

extern	cvar_t	*fx_Debug;
extern	cvar_t	*fx_Override;
