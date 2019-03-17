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
// fx_main.c Run FX Scripts and provide several commands for them

#include "client.h"
#include "fx_local.h"

fxMain_t fx;
cvar_t	*fx_Megs;
cvar_t	*fx_Vibrate;
cvar_t	*fx_Override;
cvar_t	*fx_Debug;


static ID_INLINE void fxMaskInput( const byte *mask, const void *readV, void *writeV )
{
	const long *read = (long *)readV;
	long *write = (long*)writeV;

	read += mask[0];
	mask++;
	while (1) {
		int count;
		/* Copy blocks */
		if (!mask[0])
			break;
		count = mask[0];
		do {
			*write++ = *read++;
			count--;
		} while (count > 0);
		/* Skip blocks */
		if (!mask[1])
			break;
		read += mask[1];
		mask += 2;
	}
}


static ID_INLINE void fxMaskOutput( const byte *mask, const void *readV, void *writeV ) {
	const long *read = (long *)readV;
	long *write = (long*)writeV;

	write += mask[0];
	mask++;
	while (1) {
		int count;
		/* Copy blocks */
		if (!mask[0])
			break;
		count = mask[0];
		do {
			*write++ = *read++;
			count--;
		} while (count > 0);
		/* Skip blocks */
		if (!mask[1])
			break;
		write += mask[1];
		mask += 2;
	}
}


//Some forward defines
static void *fxGetActive( unsigned int key1, unsigned int key2, int size );
static const void *fxRun( fxRun_t *run, const void *data );

static ID_INLINE int fxRand( void ) {
	fx.seed = (69069 * fx.seed + 1);
	return fx.seed;
}
static ID_INLINE float fxRandom( void ) {
	return (fxRand() & 0xffff) * (1.0f / 0x10000);
}

/* Very simple allocation function */
static void *fxAlloc( int size ) {
	int *search = fx.allocSearch;
	int *searchStart = search;
	
	size += sizeof( int );
	//TODO make size a multiple of sizeof (void *)
	while (1) {
		//Used block
		if ( search[0] == size ) {
			search[0] = -size;
			fx.allocSearch = (int*)(((char *)search) + size);
			return search + 1;
		} else if ( search[0] > size ) {
			fx.allocSearch = (int*)(((char *)search) + size);
			fx.allocSearch[0] = search[0] - size;
			search[0] = -size;
			return search + 1;
		} else if ( search[0] < 0 ) {
			search = (int*)(((char *)search) - search[0]);
		//Last block
		} else if (!search[0] ) {
			search = fx.allocStart;
		//Free block but too small, find a free one after it and join
		} else {
			int *searchNext = (int*)(((char *)search) + search[0]);
			/* Is the next one free, if so add it */
			if ( searchNext[0] > 0  ) {
				search[0] += searchNext[0];
				/* Added the start one and still not enough room? */
				if ( searchStart == searchNext && search[0] < size )
					break;
				continue;
			} else {
				search = searchNext;
			}
		}
		/* Check if it went a full loop, out of space then */
		if ( search == searchStart) {
			break;
		}
	}
	if ( fx_Debug->integer ) {
		Com_Printf("FX_Debug:Ran out of allocation space, needed %d\n", size );
	} else {
		Com_Error( ERR_DROP, "FX Ran out of allocation space for %d bytes, increase fx_Megs\n", size );
	}
	return 0;
}

static void fxVibrate( const vec3_t position, float strength ) {
	if ( fx.vibrate.used < FX_VIBRATE_QUEUE ) {
		int i = fx.vibrate.used++;
		VectorCopy( position, fx.vibrate.queue[i].origin );
		fx.vibrate.queue[i].strength = strength;
	}
}

static void fxDoVibrate( const vec3_t origin, const vec3_t position, float strength ) {
	vec3_t dist;
	float magnitude, oldmagnitude;

	VectorSubtract( origin, position, dist );

	magnitude = 1.0f - VectorLength( dist ) / 1000.0;
	if( magnitude <= 0 )
		return;
	magnitude = strength * magnitude * magnitude;

	if( fx.vibrate.time ) {
		// Adjust for current vibration
		int time = fx.time - fx.vibrate.time;
		if( time < 20 )
			time = 20;
		oldmagnitude	= fx.vibrate.magnitude / (time / 20.0f);
	}
	else {
		oldmagnitude	= 0;
	}

	fx.vibrate.magnitude	= magnitude + oldmagnitude;
	if( fx.vibrate.magnitude < 0 )
		fx.vibrate.magnitude= 0;

	fx.vibrate.offset	= fxRandom() * 360;	// Always on the 'up' half
	fx.vibrate.time		= fx.time;
}


void FX_VibrateView( float scale, vec3_t origin, vec3_t angles ) {
	int i, time;
	float magnitude;

	scale *= fx_Vibrate->value;
	if (scale <= 0) {
		fx.vibrate.used = 0;
		return;
	}
	for ( i = 0; i < fx.vibrate.used; i++ ) {
		fxDoVibrate( origin, fx.vibrate.queue[i].origin, fx.vibrate.queue[i].strength * scale ); 
	}
	fx.vibrate.used = 0;
	if( !fx.vibrate.time )
		return;

	time = fx.time - fx.vibrate.time;
	if( time < 20 )
		time = 20;
	magnitude = ((fx.vibrate.magnitude > 100) ? 100 : fx.vibrate.magnitude) / (time / 20.0f);
	if( magnitude <= 3 )
	{
		// There's not enough vibration to care about
		fx.vibrate.time = 0;
		return;
	}

	origin[2] += magnitude * 0.3 * sin( 1.4 * (fx.vibrate.offset + sqrtf(time)) );
	angles[ROLL] += magnitude * (0.1 * sin( 0.1 * (fx.vibrate.offset + sqrtf(time)) ) - 0.05);
}

static int fxSize( void *block )
{
	return ((int*)block)[-1];
}

static void fxFree( void *block ) {
	int *alloc = ((int*)block) - 1;
	alloc[0] = -alloc[0];
}

static const void *fxRunDecal( const fxRun_t *run, const fxRunDecal_t *decal ) { 
	int i;
	vec4_t color;
	
	for (i = 0;i<4;i++) 
		color[i] = run->color[i] * ( 1.0f / 255);

	re.DecalProject( run->shader, run->origin, run->dir, color, run->rotate, run->size, decal->flags, decal->life );
	return decal + 1;
}
static const void* fxRunBeam( const fxRun_t *run, const fxRunRender_t* render ) {
	refBeam_t beam;
	
	VectorCopy( run->origin, beam.start );
	VectorAdd( beam.start, run->dir, beam.end );
	beam.width = run->size;
	beam.rotateAngle = run->rotate;
	beam.shaderTime = run->shaderTime;
	beam.shader = run->shader;
	beam.renderfx = render->renderfx;
	Byte4Copy( run->color, beam.color );
	re.AddBeamToScene( &beam );
	return render + 1;
}

static const void* fxRunSpark( const fxRun_t *run, const fxRunRender_t* render ) {
	refBeam_t spark;
	
	VectorNormalize2( run->velocity, spark.end );
	VectorMA( run->origin, -0.5*run->size, spark.end, spark.start );
	VectorMA( run->origin, 0.5*run->size, spark.end, spark.end );
	spark.width = run->width;
	spark.rotateAngle = 0;
	spark.shaderTime = run->shaderTime;
	spark.shader = run->shader;
	spark.renderfx = render->renderfx;
	Byte4Copy( run->color, spark.color );
	re.AddBeamToScene( &spark );
	return render + 1;
}

static const void* fxRunRings( const fxRun_t *run, const fxRunRender_t* render ) {
	refRings_t rings;

	VectorCopy( run->origin, rings.start );
	VectorAdd( rings.start, run->dir, rings.end );
	rings.radius = run->size;
	rings.stepSize = run->width;
	rings.shaderTime = run->shaderTime;
	rings.shader = run->shader;
	rings.renderfx = render->renderfx;
	Byte4Copy( run->color, rings.color );
	re.AddRingsToScene( &rings );
	return render + 1;
}
static const void* fxRunSprite( const fxRun_t* run, const fxRunRender_t* render ) {
	refSprite_t sprite;
	VectorCopy( run->origin, sprite.origin );
	sprite.shader = run->shader;
	sprite.radius = run->size;
	sprite.rotation = run->rotate;
	sprite.shaderTime = run->shaderTime;
	Byte4Copy( run->color, sprite.color );
	sprite.renderfx = render->renderfx;
	re.AddSpriteToScene( &sprite );
	return render + 1;
}

static const void* fxRunQuad( const fxRun_t *run, const fxRunRender_t* render ) {
	polyVert_t verts[4];
	vec3_t left, up;

	PerpendicularVector( up, run->dir );
	RotatePointAroundVector( left, run->dir, up, run->angles[ROLL] );
	CrossProduct( run->dir, left, up );
	
	VectorScale( up, run->size, up );
	VectorScale( left, -run->size, left );
	verts[0].xyz[0] = run->origin[0] + left[0] + up[0];
	verts[0].xyz[1] = run->origin[1] + left[1] + up[1];
	verts[0].xyz[2] = run->origin[2] + left[2] + up[2];
	verts[1].xyz[0] = run->origin[0] - left[0] + up[0];
	verts[1].xyz[1] = run->origin[1] - left[1] + up[1];
	verts[1].xyz[2] = run->origin[2] - left[2] + up[2];
	verts[2].xyz[0] = run->origin[0] - left[0] - up[0];
	verts[2].xyz[1] = run->origin[1] - left[1] - up[1];
	verts[2].xyz[2] = run->origin[2] - left[2] - up[2];
	verts[3].xyz[0] = run->origin[0] + left[0] - up[0];
	verts[3].xyz[1] = run->origin[1] + left[1] - up[1];
	verts[3].xyz[2] = run->origin[2] + left[2] - up[2];

	Byte4Copy( run->color, verts[0].modulate );
	Byte4Copy( run->color, verts[1].modulate );
	Byte4Copy( run->color, verts[2].modulate );
	Byte4Copy( run->color, verts[3].modulate );
	verts[0].st[0] = 0;	verts[0].st[1] = 0;
	verts[1].st[0] = 1;	verts[1].st[1] = 0;
	verts[2].st[0] = 1;	verts[2].st[1] = 1;
	verts[3].st[0] = 0;	verts[3].st[1] = 1;
	re.AddPolyToScene( run->shader, 4, verts, 1 );
	return render + 1;
}


static const void* fxRunAnglesModel( const fxRun_t *run, const fxRunRender_t* render )
{
	refModel_t model;

	memset( &model, 0, sizeof( model ));
	VectorCopy( run->origin, model.origin );
	VectorCopy( run->origin, model.oldorigin );
	
	model.hModel = run->model;
	model.customShader = run->shader;
	model.shaderTime = run->shaderTime;
	model.renderfx = render->renderfx;
	Byte4Copy( run->color, model.shaderRGBA );

	// Setup the axis from the angles
	AnglesToAxis( run->angles, model.axis );
	if ( run->size > 0 && run->size != 1 ) {
		VectorScale( model.axis[0], run->size, model.axis[0] );
		VectorScale( model.axis[1], run->size, model.axis[1] );
		VectorScale( model.axis[2], run->size, model.axis[2] );
		model.nonNormalizedAxes = qtrue;
	}
	re.AddModelToScene( &model );
	return render + 1;
}

static const void* fxRunDirModel( const fxRun_t *run, const fxRunRender_t* render ) {
	refModel_t model;

	memset( &model, 0, sizeof( model ));
	VectorCopy( run->origin, model.origin );
	VectorCopy( run->origin, model.oldorigin );
	
	model.hModel = run->model;
	model.customShader = run->shader;
	model.shaderTime = run->shaderTime;
	model.renderfx = render->renderfx;
	Byte4Copy( run->color, model.shaderRGBA );

	// Setup the axis from the angles
	VectorCopy( run->dir, model.axis[0] );
	RotateAroundDirection( model.axis, run->rotate );
	if ( run->size != 1 ) {
		VectorScale( model.axis[0], run->size, model.axis[0] );
		VectorScale( model.axis[1], run->size, model.axis[1] );
		VectorScale( model.axis[2], run->size, model.axis[2] );
		model.nonNormalizedAxes = qtrue;
	}
	re.AddModelToScene( &model );
	return render + 1;
}
static const void* fxRunAxisModel( const fxRun_t *run, const fxRunRender_t* render ) {
	refModel_t model;

	memset( &model, 0, sizeof( model ));
	VectorCopy( run->origin, model.origin );
	VectorCopy( run->origin, model.oldorigin );
	
	model.hModel = run->model;
	model.customShader = run->shader;
	model.shaderTime = run->shaderTime;
	model.renderfx = render->renderfx;
	Byte4Copy( run->color, model.shaderRGBA );

	// Setup the axis from the angles
	if ( run->size != 1 ) {
		VectorScale( run->axis[0], run->size, model.axis[0] );
		VectorScale( run->axis[1], run->size, model.axis[1] );
		VectorScale( run->axis[2], run->size, model.axis[2] );
		model.nonNormalizedAxes = qtrue;
	} else {
		VectorCopy( run->axis[0], model.axis[0] );
		VectorCopy( run->axis[1], model.axis[1] );
		VectorCopy( run->axis[2], model.axis[2] );
		model.nonNormalizedAxes = qfalse;
	}
	re.AddModelToScene( &model );
	return render + 1;
}
static void fxRunLight( fxRun_t *run ) {
	float r = run->color[0] * ( 1 / 255.0f );
	float g = run->color[1] * ( 1 / 255.0f );
	float b = run->color[2] * ( 1 / 255.0f );
	re.AddLightToScene( run->origin, run->size, r, g, b);
}

static void fxRunTrace( fxRun_t *run ) {
	trace_t tr;
	vec3_t end;

	VectorMA( run->origin, 8000, run->dir, end );
	CM_BoxTrace( &tr, run->origin, end, 0, 0, 0, CONTENTS_SOLID, qfalse );
	if ( tr.fraction < 1 ) {
		VectorCopy( tr.endpos, run->origin );
		VectorCopy( tr.plane.normal, run->dir );
	}
}

static const void *fxRunShader( fxRun_t *run, const fxRunShader_t *shader ) {
	run->shader = shader->shader;
	return shader + 1;
}
static const void *fxRunShaderList( fxRun_t *run, const fxRunShaderList_t *shaderList ) {
	float i;
	qhandle_t *list = (qhandle_t*)fxRunMath( run, shaderList + 1, &i );
	run->shader = list[ (unsigned int)(i * shaderList->count) % shaderList->count ];
	return list + shaderList->count;
}
static const void *fxRunModel( fxRun_t *run, const fxRunModel_t *model ) {
	run->model = model->model;
	return model + 1;
}
static const void *fxRunModelList( fxRun_t *run, const fxRunModelList_t *modelList ) {
	float i;
	qhandle_t *list = (qhandle_t*)fxRunMath( run, modelList + 1, &i );
	run->model = list[ (unsigned int)(i * modelList->count) % modelList->count ];
	return list + modelList->count;
}
static const void *fxRunSoundList( const fxRun_t *run, const fxRunSoundList_t *sound ) {
	unsigned int index = fxRand() % sound->count;
	S_StartSound( run->origin, 0, 0, sound->handle[index] );	
	return &sound->handle[ sound->count ];
}
static const void *fxRunSound( const fxRun_t *run, const fxRunSound_t *sound ) {
	S_StartSound( run->origin, 0, 0, sound->handle );	
	return sound + 1;
}
static const void *fxRunLoopSound( const fxRun_t *run, const fxRunLoopSound_t *loopSound ) {
	S_LoopingSound( (void *)run->key, run->origin, run->velocity, loopSound->handle, 127 );
	return loopSound + 1;
}
static const void *fxRunColor( fxRun_t *run, const fxRunColor_t *color ) {
	run->color[0] = color->value[0];
	run->color[1] = color->value[1];
	run->color[2] = color->value[2];
	return color + 1;
}
static const void *fxRunColorList( fxRun_t *run, const fxRunColorList_t *colorList ) {
	float i;
	color4ub_t *list = (color4ub_t*)fxRunMath( run, colorList + 1, &i );
	byte *newColor;

	newColor = list[(unsigned int)(i * colorList->count) % colorList->count ];
	
	run->color[0] = newColor[0];
	run->color[1] = newColor[1];
	run->color[2] = newColor[2];
	return list + colorList->count;
}

static const void *fxRunColorBlend( fxRun_t *run, const fxRunColorBlend_t *colorBlend )
{
	float i;

	color4ub_t *list = (color4ub_t*)fxRunMath( run, colorBlend + 1, &i );
	const byte *start, *end;

	i *= colorBlend->count-1;
	unsigned int index = (int)( i );
	start = list[ index % colorBlend->count ];
	end = list[ (index + 1) % colorBlend->count ];
	int blend = (int)(i * 256.0f) & 255;

	run->color[0] = start[0] + ((( end[0] - start[0] ) * blend) >> 8);
	run->color[1] = start[1] + ((( end[1] - start[1] ) * blend) >> 8);
	run->color[2] = start[2] + ((( end[2] - start[2] ) * blend) >> 8);
	return list + colorBlend->count;
}

static const void *fxRunColorHue( fxRun_t *run, const void *data ) {
	float i;
	const void *next = fxRunMath( run, data, &i );
	float deg, angle, v;

	deg = (i * 360);
	angle = DEG2RAD( fmodf( deg, 120));
	v = ((cos(angle) / cos((M_PI / 3) - angle)) + 1) / 3;
	if ( deg <= 120) {
		run->color[0] = (v*255);
		run->color[1] = ((1-v)*255);
		run->color[2] = 0;
	} else if ( deg <= 240) {
		run->color[0] = 0;
		run->color[1] = (v*255);
		run->color[2] = ((1-v)*255);
	} else {
		run->color[0] = ((1-v)*255);
		run->color[1] = 0;
		run->color[2] = (v*255);
	}
	return next;
}

static const void *fxRunColorMath( fxRun_t *run, const fxRunColorMath_t *colorMath )
{
	float i;
	const void *next = fxRunMath( run, colorMath + 1, &i );
	byte *color = run->color + colorMath->index;
	if ( i < 0)
		color[0] = 0;
	else if ( i > 1.0f)
		color[0] = 255;
	else 
		color[0] = 255 * i;
	return next;
}

static const void *fxRunColorScale( fxRun_t *run, const void *data ) {
	float i;
	const void *next = fxRunMath( run, data, &i );
	int s = 255 * i;
	run->color[0] = (s * run->color[0]) >> 8;
	run->color[1] = (s * run->color[1]) >> 8;
	run->color[2] = (s * run->color[2]) >> 8;
	return next;
}

static const void *fxRunColorFade( fxRun_t *run, const fxRunColorFade_t *colorFade ) {
	if ( run->lerp > colorFade->delay ) {
		int s = 255 - (run->lerp - colorFade->delay) * 255 * (1.0f / (1.0f - colorFade->delay));
		run->color[0] = (s * run->color[0]) >> 8;
		run->color[1] = (s * run->color[1]) >> 8;
		run->color[2] = (s * run->color[2]) >> 8;
	}
	return colorFade + 1;
}

static const void *fxRunAlphaFade( fxRun_t *run, const fxRunAlphaFade_t *alphaFade ) {
	if ( run->lerp > alphaFade->delay ) {
		unsigned int s = 255 + (int)( (run->lerp - alphaFade->delay) * alphaFade->scale);
		run->color[3] = (s * run->color[3]) >> 8;
	}
	return alphaFade + 1;
}

static const void *fxRunVibrate( fxRun_t *run, const fxRunVibrate_t *vibrate) {
	fxVibrate( run->origin, vibrate->strength );
	return vibrate + 1;
}

static const void *fxRunScale( fxRun_t *run, const fxRunScale_t *scale) {
	const float *src = fxFloatSrc( run, scale->src );
	float *dst = fxFloatDst( run, scale->dst );
	float v;
	const void *next;

	next = fxRunMath( run, scale+1, &v );
	VectorScale( src, v, dst );
	return next;
}

static const void *fxRunValue( fxRun_t *run, const fxRunValue_t *value) {
	float *dst = fxFloatDst( run, value->dst );
	return fxRunMath( run, value + 1, dst );
}

static const void *fxRunClear( fxRun_t *run, const fxRunClear_t *clear) {
	float *dst = fxFloatDst( run, clear->dst );
	VectorClear( dst );
	return clear + 1;
}

static const void *fxRunAddScale( fxRun_t *run, const fxRunAddScale_t *addScale) {
	const float *src = fxFloatSrc( run, addScale->src );
	const float *scale = fxFloatSrc( run, addScale->scale );
	float *dst = fxFloatDst( run, addScale->dst );
	float v;
	const void *next;

	next = fxRunMath( run, addScale+1, &v );
	VectorMA( src, v, scale, dst );
	return next;
}

static const void *fxRunSubScale( fxRun_t *run, const fxRunSubScale_t *subScale) {
	const float *src = fxFloatSrc( run, subScale->src );
	const float *scale = fxFloatSrc( run, subScale->scale );
	float *dst = fxFloatDst( run, subScale->dst );
	float v;
	const void *next;

	next = fxRunMath( run, subScale+1, &v );
	VectorMA( src, -v, scale, dst );
	return next;
}


static const void *fxRunAdd( fxRun_t *run, const fxRunAdd_t *add) {
	const float *src1 = fxFloatSrc( run, add->src1 );
	const float *src2 = fxFloatSrc( run, add->src2 );
	float *dst = fxFloatDst( run, add->dst );
	VectorAdd( src1, src2, dst );
	return add + 1;
}

static const void *fxRunSub( fxRun_t *run, const fxRunSub_t *sub) {
	const float *src1 = fxFloatSrc( run, sub->src1 );
	const float *src2 = fxFloatSrc( run, sub->src2 );
	float *dst = fxFloatDst( run, sub->dst );
	VectorSubtract( src1, src2, dst );
	return sub + 1;
}

static const void *fxRunRotateAround( fxRun_t *run, const fxRunRotateAround_t *rotateAround) {
	const float *src = fxFloatSrc( run, rotateAround->src );
	const float *dir = fxFloatSrc( run, rotateAround->dir );
	float *dst = fxFloatDst( run, rotateAround->dst );
	float v;
	const void *next;

	next = fxRunMath( run, rotateAround+1, &v );
	RotatePointAroundVector( dst, dir, src, v );
	return next;
}

static const void *fxRunCopy( fxRun_t *run, const fxRunCopy_t *copy) {
	const float *src = fxFloatSrc( run, copy->src );
	float *dst = fxFloatDst( run, copy->dst );

	VectorCopy( src, dst );
	return copy + 1;
}

static const void *fxRunInverse( fxRun_t *run, const fxRunInverse_t *inverse ) {
	const float *src = fxFloatSrc( run, inverse->src );
	float *dst = fxFloatDst( run, inverse->dst );

	dst[0] = -src[0];
	dst[1] = -src[1];
	dst[2] = -src[2];
	return inverse+1;
}

static const void *fxRunNormalize( fxRun_t *run, const fxRunNormalize_t *normalize ) {
	const float *src = fxFloatSrc( run, normalize->src );
	float *dst = fxFloatDst( run, normalize->dst );

	VectorNormalize2( src, dst );
	return normalize+1;
}

static const void *fxRunPerpendicular( fxRun_t *run, const fxRunPerpendicular_t *perpendicular ) {
	const float *src = fxFloatSrc( run, perpendicular->src );
	float *dst = fxFloatDst( run, perpendicular->dst );

	PerpendicularVector( dst, src );
	return perpendicular+1;
}

static const void *fxRunWobble( fxRun_t *run, const fxRunWobble_t *wobble ) {
	const float *src = fxFloatSrc( run, wobble->src );
	float *dst = fxFloatDst( run, wobble->dst );
	float v;
	const void *next;
	float angle, s;
	vec3_t axis[3];

	next = fxRunMath( run, wobble+1, &v );

	/* Check for the unlikely upward/downward vector */

	VectorCopy( src, axis[0] );
	RotateAroundDirection( axis, fxRandom() * 360 ); 
	angle = DEG2RAD( v );
	s = cos( angle );
	VectorScale( axis[0], s, dst );
	s = sin( angle );
	VectorMA( dst, s, axis[2], dst );
	return next;
}

static const void *fxRunMakeAngles( fxRun_t *run, const fxRunMakeAngles_t *makeAngles ) {
	const float *src = fxFloatSrc( run, makeAngles->src );
	float *dst = fxFloatDst( run, makeAngles->dst );

	vectoangles( src, dst );
	return makeAngles + 1;
}

static const void *fxRunRandom( fxRun_t *run, const fxRunRandom_t *random ) {
	float *dst = fxFloatDst( run, random->dst );

	float yaw = fxRandom() * 2 * M_PI;
	float pitch = fxRandom() * 2 * M_PI;
	float cp = cos ( pitch );
	float sp = sin ( pitch );
	float cy = cos ( yaw );
	float sy = sin ( yaw );

	dst[0] = cp*cy;
	dst[1] = cp*sy;
	dst[2] = -sp;
	return random + 1;
}

static const void *fxRunEmitter( fxRun_t *run, const fxRunEmitter_t *emitter ) {
	fxEntity_t	*entity;

	entity = fxAlloc( emitter->allocSize );
	if ( entity ) { 
		float life;

		entity->next = fx.entityNew;
		fx.entityNew = entity;

		entity->startTime = fx.time;
		fxRunMath( run, emitter + 1, &life );
		entity->lifeScale = 1.0f / (life * 1000);
		entity->traceTime = 0;
		entity->moveTime = 0;
		entity->flags = 0;
		VectorCopy( run->origin, entity->origin );
		VectorCopy( run->velocity, entity->velocity );
		entity->emitter = emitter;
		fxMaskInput( emitter->mask, run->velocity + 3, entity + 1 );
	}
	return ((byte *)emitter) + emitter->size;
}

const void *fxRunRepeat( fxRun_t *run, const fxRunRepeat_t *repeat )
{
    float v;

	const void* next = fxRunMath( run, repeat + 1, &v );
	int count = v;
	float loopScale = 1.0 / count;
    int i;
	for (i = 0; i < count; i++ )
    {
		run->loop = i * loopScale;
		fxRun( run, next );
	}
	return ((char *)repeat) + repeat->size;
 }

const void *fxRunIf( fxRun_t *run, const fxRunIf_t *runIf ) {
	int i;
	const void *next;
	float v;

	next = runIf + 1;
	for ( i = 0; i<runIf->testCount;i++) {
		int *blockSize;
		next = fxRunMath( run, next, &v );
		blockSize = (int *)next;
		next = blockSize + 1;
		if ( v ) {
			fxRun( run, next );
			return ((char *)runIf) + runIf->size;
		} else {
			/* Skip the block for this if case */
			next = ((char *)(next)) + blockSize[0];
		}
	}
	if ( runIf->elseStep) {
		return next;
	} else {
		return ((char *)runIf) + runIf->size;
	}
}

static const void *fxRunSkip( fxRun_t *run, const fxRunSkip_t *skip ) {
	return ((byte *)skip) + skip->size;
}

static const void *fxRunDistance( fxRun_t *run, const fxRunDistance_t *distance ) {
	fxActiveDistance_t	*active;
	vec3_t delta, realOrigin;
	float lengthScale;

	if ( run->entity && run->entity->flags & FX_ENT_STATIONARY )
		return ((byte *)distance) + distance->size;
	active = fxGetActive( run->key, (unsigned int)distance, sizeof( fxActiveDistance_t) );
	if ( !active )
		return ((byte *)distance) + distance->size;
	if ( !active->beenUsed ) {
		active->beenUsed = qtrue;
		VectorCopy( run->origin, active->lastOrigin );
		return ((byte *)distance) + distance->size;
	}
	VectorSubtract( run->origin, active->lastOrigin, delta );
	lengthScale = VectorNormalize( delta );
	if ( lengthScale + active->distance <  0 ) {
		return ((byte *)distance) + distance->size;
	}
	active->distance += lengthScale;
	lengthScale = -active->distance;
	VectorCopy( run->origin, realOrigin );
	while ( active->distance >= 0 ) {
		float step;
		const void *mathEnd = fxRunMath( run, distance + 1, &step );
		VectorMA( active->lastOrigin, lengthScale, delta, run->origin );
		lengthScale += step;
		active->distance -= step;
		fxRun( run, mathEnd );
	}
	VectorCopy( realOrigin, active->lastOrigin );
	VectorCopy( realOrigin, run->origin );
	return ((byte *)distance) + distance->size;
}

static const void *fxRunInterval( fxRun_t *run, const fxRunInterval_t *interval ) {
	float i;
	fxActiveInterval_t	*active;
	const void *mathNext;

	active = fxGetActive( run->key, (unsigned int)interval, sizeof( fxActiveInterval_t) );
	if ( !active->beenUsed ) {
		mathNext = fxRunMath( run, interval + 1, &i );
		if ( i <= 0)
			goto returnIt;
		active->beenUsed = qtrue;
		fxRun( run, mathNext);
		active->nextTime = fx.time + i * 1000;
	} else while( active->nextTime <= fx.time ) {
		mathNext = fxRunMath( run, interval + 1, &i );
		if ( i <= 0)
			break;
		fxRun( run, mathNext);
		active->nextTime += i * 1000;
	}
returnIt:
	return ((byte *)interval) + interval->size;
 }

static const void *fxRunOnce( fxRun_t *run, const fxRunOnce_t *once ) {
	fxActiveOnce_t	*active;

	active = fxGetActive( run->key, (unsigned int)once, sizeof( fxActiveOnce_t) );
	if ( !active->beenUsed ) {
		active->beenUsed = qtrue;
		fxRun( run, once + 1);
	}
	return ((byte *)once) + once->size;
 }

static const void *fxRunPushParent( fxRun_t *run, const fxRunPushParent_t *push ) {
	int i;
	float *input = ( float *)(((char *)(run->parent)) + push->offset);
	if ( run->stackUsed + push->count > FX_STACK_SIZE ) {
		Com_Printf( "Overflowing run stack\n" );	
		return push + 1;
	}
	for (i = push->count; i > 0; i--) {
		run->stackData[run->stackUsed++] = *input++;
	}
	return push + 1;
}

static const void *fxRunPush( fxRun_t *run, const fxRunPush_t *push ) {
	int i;
	float *input = ( float *)(((char *)run) + push->offset);
	if ( run->stackUsed + push->count > FX_STACK_SIZE ) {
		Com_Printf( "Overflowing run stack\n" );	
		return push + 1;
	}
	for (i = push->count; i > 0; i--) {
		run->stackData[run->stackUsed++] = *input++;
	}
	return push + 1;
}

static const void *fxRunPop( fxRun_t *run, const fxRunPop_t *pop ) {
	int i;
	float *output = ( float *)(((char *)run) + pop->offset);
	if ( run->stackUsed - pop->count < 0) {
		Com_Printf( "underflowing run stack\n" );	
		return pop + 1;
	}
	for (i = pop->count; i > 0; i--) {
		output[i-1] = run->stackData[--run->stackUsed];	
	}
	return pop + 1;
}

static const void *fxRun( fxRun_t *run, const void *data );

static const void *fxRunScript( fxRun_t *run, const fxRunScript_t *script ) {
	fxRun( run, script->data );
	return script + 1;
}

static const void *fxRun( fxRun_t *run, const void *data ) {
	while ( 1 ) {
		const int *cmdList = (int *)data;
		int cmd = cmdList[0];		
		data = cmdList + 1;
		switch ( cmd ) {
		case fxCmdHalt:
			return data;
		default:
			Com_Error( ERR_DROP, "FX:Unknown command %d\n", cmd );
			break;
		case fxCmdSkip:
			data = fxRunSkip( run, (fxRunSkip_t*)data); 
			break;
		case fxCmdKill:
			if ( run->entity )
				run->entity->flags |= FX_ENT_DEAD;
			return data;
		case fxCmdEmitter:
			data = fxRunEmitter( run, (fxRunEmitter_t*)data);
			break;
		case fxCmdRepeat:
			data = fxRunRepeat( run, (fxRunRepeat_t*)data);
			break;
		case fxCmdDistance:
			data = fxRunDistance( run, (fxRunDistance_t*)data);
			break;
		case fxCmdInterval:
			data = fxRunInterval( run, (fxRunInterval_t*)data);
			break;
		case fxCmdIf:
			data = fxRunIf( run, (fxRunIf_t*)data);
			break;
		case fxCmdOnce:
			data = fxRunOnce( run, (fxRunOnce_t*)data);
			break;

		case fxCmdLight:
			fxRunLight( run );
			break;
		case fxCmdSprite:
			data = fxRunSprite( run, (fxRunRender_t*)data );
			break;
		case fxCmdQuad:
			data = fxRunQuad( run, (fxRunRender_t*)data );
			break;
		case fxCmdSpark:
			data = fxRunSpark( run, (fxRunRender_t*)data );
			break;
		case fxCmdBeam:
			data = fxRunBeam( run, (fxRunRender_t*)data );
			break;
		case fxCmdRings:
			data = fxRunRings( run, (fxRunRender_t*)data );
			break;
		case fxCmdAxisModel:
			data = fxRunAxisModel( run, (fxRunRender_t*)data );
			break;
		case fxCmdAnglesModel:
			data = fxRunAnglesModel( run, (fxRunRender_t*)data );
			break;
		case fxCmdDirModel:
			data = fxRunDirModel( run, (fxRunRender_t*)data );
			break;
		case fxCmdDecal:
			data = fxRunDecal( run, (fxRunDecal_t*)data );
			break;

		case fxCmdTrace:
			fxRunTrace( run );
			break;

		case fxCmdScript:
			data = fxRunScript( run, (fxRunScript_t*)data );
			break;
		case fxCmdColor:
			data = fxRunColor( run, (fxRunColor_t*)data); 
			break;
		case fxCmdColorList:
			data = fxRunColorList( run, (fxRunColorList_t*)data); 
			break;
		case fxCmdColorBlend:
			data = fxRunColorBlend( run, (fxRunColorBlend_t*)data); 
			break;
		case fxCmdColorMath:
			data = fxRunColorMath( run, (fxRunColorMath_t*)data);
			break;
		case fxCmdColorScale:
			data = fxRunColorScale( run, data);
			break;
		case fxCmdColorHue:
			data = fxRunColorHue( run, data);
			break;
		case fxCmdShader:
			data = fxRunShader( run, (fxRunShader_t*)data); 
			break;
		case fxCmdShaderList:
			data = fxRunShaderList( run, (fxRunShaderList_t*)data); 
			break;
		case fxCmdModel:
			data = fxRunModel( run, (fxRunModel_t*)data); 
			break;
		case fxCmdModelList:
			data = fxRunModelList( run, (fxRunModelList_t*)data); 
			break;
		case fxCmdSound:
			data = fxRunSound( run, (fxRunSound_t*)data); 
			break;
		case fxCmdSoundList:
			data = fxRunSoundList( run, (fxRunSoundList_t*)data); 
			break;
		case fxCmdLoopSound:
			data = fxRunLoopSound( run, (fxRunLoopSound_t*)data); 
			break;
		case fxCmdVibrate:
			data = fxRunVibrate( run, (fxRunVibrate_t*)data); 
			break;

		case fxCmdAlphaFade:
			data = fxRunAlphaFade( run, (fxRunAlphaFade_t*)data); 
			break;
		case fxCmdColorFade:
			data = fxRunColorFade( run, (fxRunColorFade_t*)data); 
			break;
		case fxCmdPush:
			data = fxRunPush( run, (fxRunPush_t*)data); 
			break;
		case fxCmdPop:
			data = fxRunPop( run, (fxRunPop_t*)data); 
			break;
		case fxCmdPushParent:
			data = fxRunPushParent( run, (fxRunPushParent_t*)data); 
			break;

		case fxCmdAdd:
			data = fxRunAdd( run, (fxRunAdd_t*)data);
			break;
		case fxCmdSub:
			data = fxRunSub( run, (fxRunSub_t*)data);
			break;
		case fxCmdAddScale:
			data = fxRunAddScale( run, (fxRunAddScale_t*)data);
			break;
		case fxCmdSubScale:
			data = fxRunSubScale( run, (fxRunSubScale_t*)data);
			break;
		case fxCmdRotateAround:
			data = fxRunRotateAround( run, (fxRunRotateAround_t*)data);
			break;
		case fxCmdScale:
			data = fxRunScale( run, (fxRunScale_t*)data);
			break;
		case fxCmdCopy:
			data = fxRunCopy( run, (fxRunCopy_t*)data);
			break;
		case fxCmdRandom:
			data = fxRunRandom( run, (fxRunRandom_t*)data);
			break;
		case fxCmdWobble:
			data = fxRunWobble( run, (fxRunWobble_t*)data);
			break;
		case fxCmdNormalize:
			data = fxRunNormalize( run, (fxRunNormalize_t*)data);
			break;
		case fxCmdPerpendicular:
			data = fxRunPerpendicular( run, (fxRunPerpendicular_t*)data);
			break;
		case fxCmdInverse:
			data = fxRunInverse( run, (fxRunInverse_t*)data);
			break;
		case fxCmdClear:
			data = fxRunClear( run, (fxRunClear_t*)data);
			break;
		case fxCmdValue:
			data = fxRunValue( run, (fxRunValue_t*)data );
			break;
		}
	}
	return data;
}

static fxEntity_t *fxRunEntities( fxEntity_t **nextEntity ) {
	fxEntity_t	*lastEntity;
	fxRun_t		run;

	lastEntity = 0;
	for (; nextEntity[0]; nextEntity = &nextEntity[0]->next ) {
		float deltaTime;
		float moveTime;
		int movePasses;
		const fxRunEmitter_t *emitter;
killSkip:
		run.entity = nextEntity[0];

		fx.last.entityCount++;
		fx.last.entitySize += fxSize( run.entity );

		deltaTime = (fx.time - run.entity->startTime) + fx.timeFraction;
		run.lerp = deltaTime * run.entity->lifeScale;
		if ( run.lerp >= 1.0f ) {
			run.entity->flags |= FX_ENT_DEAD;
		} else if ( deltaTime < 0 ) {
			lastEntity = run.entity;
			continue;
		}
		run.shaderTime = run.entity->startTime;
		run.life = deltaTime * 0.001;
		run.key = (unsigned int)run.entity;
		emitter = run.entity->emitter;
		run.stackUsed = 0;

		if ( !(emitter->flags & FX_EMIT_MOVE ) ) {
			VectorCopy( run.entity->origin, run.origin );
			VectorCopy( run.entity->velocity, run.velocity );
			goto runEntity; 
		} else if ( !(emitter->flags & FX_EMIT_TRACE ) ) {
			VectorMA( run.entity->origin, run.life, run.entity->velocity, run.origin );
			VectorCopy( run.entity->velocity, run.velocity );
		
			run.velocity[2] -= 2 * run.life * emitter->gravity;
			run.origin[2] -= run.life * run.life * emitter->gravity;
			goto runEntity; 
		} else if ( run.entity->flags & FX_ENT_STATIONARY ) {
			VectorCopy( run.entity->origin, run.origin );
			if ( emitter->flags & FX_EMIT_SINK && run.lerp > emitter->sinkDelay ) {
				run.origin[2] -= emitter->sinkDepth * ( run.lerp - emitter->sinkDelay );
			}
			VectorClear( run.velocity );
			goto runEntity; 
		}

		moveTime = run.life - run.entity->moveTime;
		//Still within the valid trace range just move it directly
		if( !moveTime || (moveTime < run.entity->traceTime )) {
traceOkay:
			VectorMA( run.entity->origin, moveTime, run.entity->velocity, run.origin );
			run.origin[2] -= moveTime * moveTime * emitter->gravity;
			VectorCopy( run.entity->velocity, run.velocity );
			run.velocity[2] -= 2 * moveTime * emitter->gravity;
			goto runEntity; 
		}

		//movepasses will go down fast incase stuff fails
		for( movePasses = 2; movePasses >= 0; movePasses-- ) {
			trace_t tr;
			vec3_t oldOrigin, oldVelocity, newOrigin;
			vec3_t traceMin, traceMax;
			float addTime;
	
			//Tracetime holds the time since the last update to the origin/velocity
			//Calculate the previous trace end position
			VectorMA( run.entity->origin, run.entity->traceTime, run.entity->velocity, oldOrigin );
			oldOrigin[2] -= run.entity->traceTime * run.entity->traceTime * emitter->gravity;
			VectorCopy( run.entity->velocity, oldVelocity );
			oldVelocity[2] -= 2 * run.entity->traceTime * emitter->gravity; 

			//Predict how far we can go from this position if there's any time left in the particle's life anyway
			addTime = 0.001 * movePasses * ( 30 + (rand() & 15 ) );

			//If the entity is still moving upwards check when upward speed will hit 0 if that takes longer that 2 milliseconds
			if ( oldVelocity[2] > 0 ) {
				float left = oldVelocity[2] / ( 2 * emitter->gravity );
				if ( left < addTime ) {
					addTime = left;
				}
				traceMin[2] = 0;
				traceMax[2] = addTime * addTime * emitter->gravity;
			} else {
				traceMin[2] = -addTime * addTime * emitter->gravity;
				traceMax[2] = 0;
			}
			traceMin[0] = traceMin[1] = -0.1f; 
			traceMin[2] -= 0.1f;
			traceMax[0] = traceMax[1] = 0.1f;
			traceMax[2] += 0.1f;
			//Check the end position of the trace without the last piece of gravity
			VectorMA( oldOrigin, addTime, oldVelocity, newOrigin );

			CM_BoxTrace( &tr, oldOrigin, newOrigin, traceMin, traceMax, 0, CONTENTS_SOLID, qfalse );
			if ( tr.allsolid || tr.fraction == 0 )
				break;
			if ( tr.fraction < 1 ) {
				addTime *= tr.fraction * 0.99;
			}
			run.entity->traceTime += addTime;
			//Have we moved into safe territory for now
			if ( run.entity->traceTime >= moveTime ) {
				goto traceOkay;
			}
			//Did we only add a very small piece go into regular trace mode
//			if ( addTime > 0.001f )
//				break;
		}

		//Can do a few passes if the particle bounces between a few surfaces
		for( movePasses = 2; movePasses >= 0; movePasses-- ) {
			vec3_t oldOrigin;
			trace_t tr;

			VectorMA( run.entity->origin, run.entity->traceTime, run.entity->velocity, oldOrigin );
			oldOrigin[2] -= run.entity->traceTime * run.entity->traceTime * emitter->gravity;

			VectorCopy( run.entity->origin, run.origin );
			VectorMA( run.entity->origin, moveTime, run.entity->velocity, run.origin );
			run.origin[2] -= moveTime * moveTime * emitter->gravity;

			VectorCopy( run.entity->velocity, run.velocity );
			run.velocity[2] -= 2 * moveTime * emitter->gravity;
	
			//Determine how far we can still go before we hit something
			CM_BoxTrace( &tr, oldOrigin, run.origin, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qtrue );
			if ( tr.allsolid || (tr.surfaceFlags & SURF_NOIMPACT) ) {
				/* Special case just don't do anything */
				goto killEntity;
			//Full trace succeeded
			} else if ( tr.fraction == 1 ) {
				goto runEntity;
			} else {
				if ( emitter->flags & FX_EMIT_IMPACT ) {
					run.entity->flags |= FX_ENT_DEAD;
					goto runEntity;
				}
				if ( emitter->impactRun && (VectorLengthSquared( run.velocity ) > emitter->impactSpeed )  ) {
					/* Run the optional code for the impact handler */
					VectorCopy( tr.endpos, run.origin );
					VectorCopy( tr.plane.normal, run.dir );
					fxRun( &run, ((char *)emitter) + emitter->impactRun );
				} 
				if ( emitter->bounceFactor > 0 ) {
					float bounceTime;
					float reflect;	
					float fractionTime = ( moveTime - run.entity->traceTime ) * tr.fraction;

					bounceTime = run.entity->traceTime + fractionTime;
					if ( tr.fraction ) {
						//Add back the velocity till the impact time
						run.velocity[2] += 2 * fractionTime * emitter->gravity;
						reflect = -2 * DotProduct( tr.plane.normal, run.velocity );
						VectorMA( run.velocity, reflect, tr.plane.normal, run.velocity );
						VectorScale( run.velocity, emitter->bounceFactor, run.velocity );
						VectorCopy( run.velocity, run.entity->velocity );
					}
					VectorCopy( tr.endpos, run.entity->origin );

					//Stop moving if it's on a vertical slope and speed is low */
					if ( ( tr.plane.normal[2] > 0 && VectorLengthSquared( run.velocity ) < 40 ) ) {
						run.entity->flags |= FX_ENT_STATIONARY;
						VectorClear( run.entity->velocity );
						goto runEntity;
					}
					//Move slightly back in time to prevent negative offsets
					run.entity->moveTime += bounceTime;
					moveTime = run.life - run.entity->moveTime;
					run.entity->traceTime = 0;
					continue;
				}
			}
		}
runEntity:
		fxMaskOutput( emitter->mask, run.entity + 1, run.velocity + 3 );
		if ( run.entity->flags & FX_ENT_DEAD ) {
			if ( emitter->deathRun )
				fxRun( &run, ((char *)emitter) + emitter->deathRun );
			goto killEntity;
		} else {
			fxRun( &run, ((char *)emitter) + emitter->emitRun );
		}
		lastEntity = run.entity;
		continue;
killEntity:
		nextEntity[0] = run.entity->next;
		fxFree( run.entity );
		if ( !nextEntity[0] )
			break;
		goto killSkip;
	}
	return lastEntity;
}

static void *fxGetActive( unsigned int key1, unsigned int key2, int size ) {
	fxActive_t *active;
	unsigned int index;

	index = ((key1 ^ key2) >> FX_ACTIVE_HASH_SHIFT) ^ key1 ^ key2;
	index &= FX_ACTIVE_HASH_MASK;
	active = fx.activeHash[ index ];
	while (1) {
		if (!active) {
			active = fxAlloc( sizeof( fxActive_t ) + size );
			if (!active)
				return 0;
			active->next = fx.activeHash[ index ];
			fx.activeHash[ index ] = active;

			active->key1 = key1;
			active->key2 = key2;
			memset( active + 1, 0, size );
			fx.last.activeCount++;
			fx.last.activeSize += fxSize( active );
			break;
		} else if (active->key1 == key1 && active->key2 == key2) {
			fx.last.activeCount++;
			fx.last.activeSize += fxSize( active );
			break;
		}
		active = active->next;
	}
	active->beenUsed = qtrue;
	return active + 1;
}


static void fxDebugDrawString( int x, int y, const char *line ) {
	int i;
	for (i = 0; i<40;i++) {
		if (!line[i])
			break;
		SCR_DrawSmallChar( x, y, line[i] );
		x += SMALLCHAR_WIDTH;
	}
}

static void fxDebugDraw( void ) {
	char buf[512];
	int x = cls.glconfig.vidWidth * 3 / 4;
	int y = cls.glconfig.vidHeight - SMALLCHAR_HEIGHT * 10 * 1.5;
	
	re.SetColor( colorWhite );

	Com_sprintf( buf, sizeof( buf ), "Entities %d, mem %dkb", fx.last.entityCount, fx.last.entitySize / -1024 );
	fxDebugDrawString( x, y, buf );
	y += SMALLCHAR_HEIGHT * 1.5;
	Com_sprintf( buf, sizeof( buf ), "Active %d, mem %dkb", fx.last.activeCount, fx.last.activeSize / -1024 );
	fxDebugDrawString( x, y, buf );
	y += SMALLCHAR_HEIGHT * 1.5;

	re.SetColor( NULL );
}

void FX_Debug( void ) {
	if ( fx_Debug->integer )
		fxDebugDraw();
}

void FX_Reset( void ) {
	int *lastAlloc;

	memset( fx.activeHash, 0, sizeof( fx.activeHash ));
	fx.entityActive = 0;
	fx.entityNew = 0;
	fx.vibrate.time = 0;
	fx.vibrate.used = 0;
	/* Create the terminator block */
	fx.allocStart[0] = fx.allocSize - sizeof( int );
	fx.allocSearch = fx.allocStart;
	lastAlloc = (int *)(((char *)fx.allocStart) + fx.allocSize - sizeof( int ));
	lastAlloc[0] = 0;
}

void FX_Begin( int time, float timeFraction ) {
	fx.deltaTime = fx.time - fx.oldTime;
	fx.oldTime = fx.time;
	fx.time = time;
	fx.timeFraction = timeFraction;
	if ( fx.deltaTime < 0 || fx.deltaTime > 50) {
		fx.oldTime = fx.time;
		fx.deltaTime = 0;
	}
	fx.seed = time;
	/* Clear some stats */
	fx.last.activeCount = 0;
	fx.last.entityCount = 0;
	fx.last.entitySize = 0;
	fx.last.activeSize = 0;
	/* Run the currently active entities */
	fxRunEntities( &fx.entityActive );
}

void FX_End( void ) {
	int i;
	int loops = 5;
	/* Run all the new entities and loop a few times for recursive entities */
	while ( loops>0 && fx.entityNew ) {
		fxEntity_t *newEntities, *lastEntity;
		newEntities = fx.entityNew;
		fx.entityNew = 0;
		lastEntity = fxRunEntities( &newEntities );
		if ( lastEntity ) {
			lastEntity->next = fx.entityActive;
			fx.entityActive = newEntities;
		}
		loops --;
	}
	/* Go through the active list and set all to false */
	for( i = 0; i < FX_ACTIVE_HASH_SIZE; i++) {
		fxActive_t **activeList = &fx.activeHash[i];
		while ( activeList[0] ) {
			fxActive_t *active = activeList[0];
			if ( active->beenUsed ) {
				active->beenUsed = qfalse;
				if (!active->next)
					break;
				activeList = &active->next;
				continue;
			}
			activeList[0] = active->next;
			fxFree( active );
		}
	}
}

void FX_Run( fxHandle_t handle, const fxParent_t *parent, unsigned int key ) {
	fxRun_t run;
	const fxScript_t *script;

	if (handle <= 0 || handle > fx.scriptCount || !parent )
		return;
	script = fx.scripts[handle];

	//Check for illegal readmasks for undefined values
	if ( fx_Debug->integer ) {
		int debug = fx_Debug->integer;
		char warning[256];
		warning[0] = 0;
		if ( (script->readMask & FX_MASK_ORIGIN) && !(parent->flags & FXP_ORIGIN ) ) {
			Q_strcat( warning, sizeof( warning ), "origin, " );
		}
		if ( (script->readMask & FX_MASK_VELOCITY) && !(parent->flags & FXP_VELOCITY ) ) {
			Q_strcat( warning, sizeof( warning ), "velocity, " );
		}
		if ( (script->readMask & FX_MASK_DIR) && !(parent->flags & FXP_DIR ) ) {
			Q_strcat( warning, sizeof( warning ), "dir, " );
		}
		if ( (script->readMask & FX_MASK_ANGLES) && !(parent->flags & FXP_ANGLES ) ) {
			Q_strcat( warning, sizeof( warning ), "angles, " );
		}
		if ( debug > 1 && (script->readMask & FX_MASK_SIZE) && !(parent->flags & FXP_SIZE ) ) {
			Q_strcat( warning, sizeof( warning ), "size, " );
		}
		if ( (script->readMask & FX_MASK_WIDTH) && !(parent->flags & FXP_WIDTH ) ) {
			Q_strcat( warning, sizeof( warning ), "width, " );
		}
		if ( debug > 1 && (script->readMask & FX_MASK_SHADER) && !(parent->flags & FXP_SHADER ) ) {
			Q_strcat( warning, sizeof( warning ), "shader, " );
		}
		if ( (script->readMask & FX_MASK_MODEL) && !(parent->flags & FXP_MODEL ) ) {
			Q_strcat( warning, sizeof( warning ), "model, " );
		}
		if ( (script->readMask & FX_MASK_AXIS) && !(parent->flags & FXP_AXIS ) ) {
			Q_strcat( warning, sizeof( warning ), "axis, " );
		}
		if ( warning[0] ) {
			Com_Printf("FX_Debug:script '%s' requires %s which aren't supplied by caller\n", script->entry->name, warning );
		}
	}
	VectorCopy( parent->origin, run.origin );
	if ( parent->flags & FXP_VELOCITY)
		VectorCopy( parent->velocity, run.velocity );
	else
		VectorClear( run.velocity );
	if ( parent->flags & FXP_DIR ) 
		VectorCopy( parent->dir, run.dir );
	if ( parent->flags & FXP_ANGLES ) {
		VectorCopy( parent->angles, run.angles );
		run.rotate = run.angles[ROLL];
	} else {
		VectorClear( run.angles );
		run.rotate = 0;
	}
	if ( parent->flags & FXP_AXIS ) 
		AxisCopy( parent->axis, run.axis );

	*(int *)run.color = ( parent->flags & FXP_COLOR ) ? *(int *)parent->color : 0xffffffff;
	run.shader = ( parent->flags & FXP_SHADER ) ? parent->shader : 0;
	run.model = ( parent->flags & FXP_MODEL ) ? parent->model : 0;
	run.size = ( parent->flags & FXP_SIZE ) ? parent->size : 1.0f;
	run.width = ( parent->flags & FXP_WIDTH ) ? parent->width : 1.0f;

	run.parent = parent;
	run.shaderTime = 0;
	run.key = key;
	run.stackUsed = 0;
	run.entity = 0;
	fxRun( &run, script->data );
}


static void fxList( void ) {
	const fxScript_t *script;
	int i;

	if ( fx.scriptCount <= 1) {
		Com_Printf("No fx scripts loaded\n");
		return;
	}
	Com_Printf ( "-----------------------\n" );
	for ( i = 1; i<fx.scriptCount;i++ ) {
		script = fx.scripts[i];
		Com_Printf( "%s", script->entry->name );
		if ( script->remap ) {
			Com_Printf( " remap to %s", script->remap->entry->name );
		}
		Com_Printf( "\n");
	}
	Com_Printf( "%d Total scripts\n", fx.scriptCount - 1);
	Com_Printf ( "-----------------------\n" );
}

static void fxStats( void ) {
	int *alloc = fx.allocStart;
	int allocSize, allocBlocks;
	int freeSize, freeBlocks;

	allocSize = allocBlocks = 0;
	freeSize = freeBlocks = 0;

	while ( 1 ) {
		int size = alloc[0];
		if ( size < 0 ) {
			size = -size;
			allocSize += size;
			allocBlocks++;
		} else if ( size > 0 ) {
			freeSize += size;
			freeBlocks++;
		} else
			break;
		alloc = (int*)(((char *)alloc) + size);
	}

	Com_Printf( "FX Stats:\n" );
	Com_Printf( "Free %d in %d blocks\n", freeSize, freeBlocks );
	Com_Printf( "Allocated %d in %d blocks\n", allocSize, allocBlocks );
}

void FX_Init( void ) {
	Com_Memset( &fx, 0, sizeof( fx ));

	fx_Megs = Cvar_Get( "fx_Megs", "2", CVAR_ARCHIVE | CVAR_LATCH );
	fx_Vibrate = Cvar_Get( "fx_Vibrate", "1", CVAR_ARCHIVE );
	fx_Override = Cvar_Get( "fx_Override", "", CVAR_ARCHIVE );
	fx_Debug = Cvar_Get( "fx_Debug", "0", CVAR_CHEAT );
	fx.allocSize = fx_Megs->integer;
	if ( fx.allocSize < 1)
		fx.allocSize = 1;
	else if (fx.allocSize > 8) {
		fx.allocSize = 8;
	}
	fx.allocSize *= 1024*1024;
	fx.allocStart = Hunk_Alloc( fx.allocSize, h_low );
	FX_Reset( );
	Cmd_AddCommand( "fxStats", fxStats );
	Cmd_AddCommand( "fxList", fxList );
	Cmd_AddCommand( "fxReset", FX_Reset );

	fxInitParser();
}

void FX_Shutdown( void ) {
	Cmd_RemoveCommand( "fxStats" );
	Cmd_RemoveCommand( "fxReset" );
	Cmd_RemoveCommand( "fxRemap" );
	Cmd_RemoveCommand( "fxMath" );
	Cmd_RemoveCommand( "fxReload" );
	if ( fx.alloc.entries )
		fxFreeMemory();
}
