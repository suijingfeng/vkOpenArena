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
#include "../renderercommon/matrix_multiplication.h"
/*
=============================================================================

LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light sources are visible.
The size of the flare relative to the screen is nearly constant, irrespective of distance, 
but the intensity should be proportional to the projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that its midpoint should have when the surface is added.

After all opaque surfaces have been rendered, the depth buffer is read back for each flare in view.
If the point has not been obscured by a closer surface, the flare should be drawn.

Surfaces that have a repeated texture should never be flagged as flaring, 
because there will only be a single flare added at the midpoint of the polygon.

To prevent abrupt popping, the intensity of the flare is interpolated up and
down as it changes visibility.  This involves scene to scene state, unlike almost
all other aspects of the renderer, and is complicated by the fact that a single
frame may have multiple scenes.

RB_RenderFlares() will be called once per view (twice in a mirrored scene,
potentially up to five or more times in a frame with 3D status bar icons).

=============================================================================
*/


// flare states maintain visibility over multiple frames for fading
// layers: view, mirror, menu
typedef struct flare_s {
	struct		flare_s	*next;		// for active chain

	int			addedFrame;

	qboolean	inPortal;			// true if in a portal view of the scene
	int			frameSceneNum;
	void		*surface;
	int			fogNum;

	int			fadeTime;

	qboolean	visible;			// state of last test
	float		drawIntensity;		// may be non 0 even if !visible due to fading

	int			windowX, windowY;
	float		eyeZ;

	vec3_t		origin;
	vec3_t		color;
	int		    radius;			// leilei - for dynamic light flares
	qboolean 	peek;

	struct shader_s* theshader;	// leilei - custom flare shaders
	int		type;			// 0 - map, 1 - dlight, 2 - sun
	float	delay;			// update delay time
} flare_t;

#define		MAX_FLARES		256 // was 128
#define     FLARE_STDCOEFF "150"

extern	shaderCommands_t	tess;


static flare_t		r_flareStructs[MAX_FLARES];
static flare_t*     r_activeFlares;
static flare_t*     r_inactiveFlares;

static int flareCoeff;
static float flaredsize;	// leilei - dirty flare fix for widescreens

// light flares
cvar_t* r_flares;
static cvar_t* r_flareMethod;	// method of flare intensity
static cvar_t* r_flareCoeff;
static cvar_t* r_flareQuality;	// testing quality of the flares.
static cvar_t* r_flareSize;
static cvar_t* r_flareFade;



/*
==========================
R_TransformClipToWindow
==========================
*/
static void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window )
{
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) ( window[0] + 0.5 );
	window[1] = (int) ( window[1] + 0.5 );
}

/*
==================
RB_AddFlare: This is called at surface tesselation time
==================
*/
void RB_AddFlare(srfFlare_t *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal, int radii, float scaled, int type)
{
	int				i;
	flare_t			*f;
	vec3_t			local;
	float			d = 1;
	vec4_t			eye, clip, normalized, window;

	backEnd.pc.c_flareAdds++;

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	if(normal && (normal[0] || normal[1] || normal[2]) )
    {
		VectorSubtract( backEnd.viewParms.or.origin, point, local );
		FastNormalize1f(local);
		d = DotProduct(local, normal);

		// If the viewer is behind the flare don't add it.
		if(d < 0)
			return;
	}

	flaredsize = backEnd.viewParms.viewportHeight;

	TransformModelToClip( point, backEnd.or.modelMatrix, backEnd.viewParms.projectionMatrix, eye, clip );


	// check to see if the point is completely off screen
	for ( i = 0 ; i < 3 ; i++ )
    {
		if ( (clip[i] >= clip[3]) || (clip[i] <= -clip[3]) )
        {
			return;
		}
	}


	R_TransformClipToWindow( clip, &backEnd.viewParms, normalized, window );

	if ( window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth || window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight )
    {
		return;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	for ( f = r_activeFlares ; f ; f = f->next )
    {
		if ( f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum
		        && f->inPortal == backEnd.viewParms.isPortal )
        {
			break;
		}
	}

	// allocate a new one
	if (!f )
    {
        // the list is completely full
		if ( !r_inactiveFlares )
			return;
	
		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next = r_activeFlares;
		r_activeFlares = f;

		f->surface = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal = backEnd.viewParms.isPortal;
		f->addedFrame = -1;
	}

	if ( f->addedFrame != backEnd.viewParms.frameCount - 1 )
    {
		f->visible = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum = fogNum;
	VectorCopy(point, f->origin);
	VectorCopy( color, f->color );



	VectorScale( f->color, d, f->color );

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];


	f->radius = radii * scaled * 0.17;
	f->eyeZ = eye[2];
	f->theshader = tr.flareShader;
	f->type = type;

	if (f->type == 0)
		f->theshader = surface->shadder;
	else
		f->theshader = tr.flareShader;

	if ( (type == 1) && (r_flaresDlightOpacity->value) )
    {	// leilei - dynamic light flare scale
		float ef = r_flaresDlightOpacity->value;
		if (ef > 1.0f)
            ef = 1.0f;
        else if (ef < 0.1f)
            ef = 0.1f;

		f->color[0] *= ef;
		f->color[1] *= ef;
		f->color[2] *= ef;
	}

}

/*
===============================================================================

FLARE BACK END

===============================================================================
*/


/*
==================
RB_TestFlareFast

faster simple one.
==================
*/
static void RB_TestFlareFast( flare_t *f, int dotrace )
{
	float			depth;
	qboolean		visible;
	float			fade;
	float			screenZ;

	backEnd.pc.c_flareTests++;


	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;
	if (f->type == 2)
        dotrace = 0; // sun cant trace


	// leilei - delay hack, to speed up the renderer

	if (backEnd.refdef.time > f->delay)
    {
		// read back the z buffer contents

		qglReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );
		f->delay = backEnd.refdef.time + r_flareDelay->value;


		screenZ = backEnd.viewParms.projectionMatrix[14] /
		          ( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

		visible = ( -f->eyeZ - -screenZ ) < 24;

		if ( visible )
        {
			if ( !f->visible )
            {
				f->visible = qtrue;
				f->fadeTime = backEnd.refdef.time - 1;
			}

			fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
		}
		else
        {
			if ( f->visible ) {
				f->visible = qfalse;
				f->fadeTime = backEnd.refdef.time - 1;
			}
			fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
		}

		if ( fade < 0 ) {
			fade = 0;
		}
		if ( fade > 1 ) {
			fade = 1;
		}

		f->drawIntensity = fade;

	}
	else
	{
        // leilei - continue drawing the flare from where we last checked
		if (f->visible)
			f->drawIntensity  = 1;
		else
			f->drawIntensity  = 0;
	}

}


static void RB_TestFlare( flare_t *f)
{
	float			depth;
	qboolean		visible;
	float			fade;
	float			screenZ;

	backEnd.pc.c_flareTests++;


	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;



	// read back the z buffer contents

	qglReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );


	screenZ = backEnd.viewParms.projectionMatrix[14] / ( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

	visible = ( -f->eyeZ - -screenZ ) < 24;


	if( visible )
    {
		if ( !f->visible )
        {
			f->visible = qtrue;
			f->fadeTime = backEnd.refdef.time - 1;
		}

		fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
	}
	else
    {
		if ( f->visible )
        {
			f->visible = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
	}

	if( fade < 0 )
    {
		fade = 0;
	}
    else if ( fade > 1 )
    {
		fade = 1;
	}

	f->drawIntensity = fade;

}



void RB_RenderFlare( flare_t *f )
{
	float			size;
	vec3_t			color;
	int				iColor[3];
	float distance, intensity;
	unsigned char fogFactors[3] = {255, 255, 255};
	int ind=0;
	int alphcal;
	backEnd.pc.c_flareRenders++;

	flaredsize = backEnd.viewParms.viewportHeight * (f->radius * 0.06);

	// We don't want too big values anyways when dividing by distance.
	if(f->eyeZ > -1.0f)
		distance = 1.0f;
	else
		distance = -f->eyeZ;

	if( (r_flaresDlightShrink->integer) && (f->type == 1) )
    {	// leilei - dynamic light flares shrinking when close

		float newd = distance / (48.0f);
		if (newd > 1)
            newd = 1.0f;

		flaredsize *= newd;
	}


	if(!f->radius)
		f->radius = 0.0f;	// leilei - don't do a radius if there is no radius at all!

	// calculate the flare size..

	/*
	 * This is an alternative to intensity scaling. It changes the size of the flare on screen instead
	 * with growing distance. See in the description at the top why this is not the way to go.
	*/
	// size will change ~ 1/r.
	if (r_flareMethod->integer == 1 || r_flareMethod->integer == 4 )
    {	// The "not the way to go" method. seen in EF
		size = flaredsize * (r_flareSize->value / (-2.0f * distance));
	}
	else if (r_flareMethod->integer == 2)
    {			// Raven method
		size = flaredsize * ( r_flareSize->value/640.0f + 8 / -f->eyeZ );
	}
	else
    {
		size = flaredsize * ( (r_flareSize->value) /640.0f + 8 / distance );
	}

	/*
	 * As flare sizes stay nearly constant with increasing distance 
     * we must decrease the intensity to achieve a reasonable visual result. 
     * The intensity is ~ (size^2 / distance^2) which can be got by considering
     * the ratio of (flaresurface on screen) : (Surface of sphere defined by flare origin and distance from flare)
	 * An important requirement is: intensity <= 1 for all distances.
	 *
	 * The formula used here to compute the intensity is as follows:
	 * intensity = flareCoeff * size^2 / (distance + size*sqrt(flareCoeff))^2
	 * As you can see, the intensity will have a max. of 1 when the distance is 0.
	 * The coefficient flareCoeff will determine the falloff speed with increasing distance.
	 */

	float factor = distance + size * sqrt(flareCoeff);

	if (r_flareMethod->integer == 4)		// leilei - EF didn't scale intensity on distance. Speed I guess
		intensity = 1;
	else
		intensity = flareCoeff * size * size / (factor * factor);

	if (r_flareMethod->integer == 1) {	// leilei - stupid hack to fix the not the way method
		if (intensity > 1)
            intensity = 1;
	}

	VectorScale(f->color, f->drawIntensity * intensity, color);



// Calculations for fogging
	if(tr.world && f->fogNum > 0 && f->fogNum < tr.world->numfogs)
    {
		tess.numVertexes = 1;
		VectorCopy(f->origin, tess.xyz[0]);
		tess.fogNum = f->fogNum;

		RB_CalcModulateColorsByFog(fogFactors);

		// We don't need to render the flare if colors are 0 anyways.
		if(!(fogFactors[0] || fogFactors[1] || fogFactors[2]))
			return;
	}

	iColor[0] = color[0] * fogFactors[0];
	iColor[1] = color[1] * fogFactors[1];
	iColor[2] = color[2] * fogFactors[2];

	alphcal = 255;				// Don't mess with alpha.


	float halfer = 1;

    if (r_flareQuality->integer)
    {	// leilei - high quality flares get no depth testing
        int index = 0;
        for(index = 0; index <f->theshader->numUnfoggedPasses; index++)
        {
            f->theshader->stages[index]->adjustColorsForFog = ACFF_NONE;
            f->theshader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
        }
    }
    RB_BeginSurface( f->theshader, f->fogNum );



	// FIXME: use quadstamp?
	tess.xyz[tess.numVertexes][0] = f->windowX - size;
	tess.xyz[tess.numVertexes][1] = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX - size;
	tess.xyz[tess.numVertexes][1] = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1 * halfer;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX + size;
	tess.xyz[tess.numVertexes][1] = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 1 * halfer;
	tess.texCoords[tess.numVertexes][0][1] = 1 * halfer;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX + size;
	tess.xyz[tess.numVertexes][1] = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 1 * halfer;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	ind+=4;

	RB_EndSurface();
}

/*
==================
RB_RenderFlares

Because flares are simulating an occular effect, they should be drawn after
everything (all views) in the entire frame has been drawn.

Because of the way portals use the depth buffer to mark off areas,
the needed information would be lost after each view, 
so we are forced to draw flares after each view.

The resulting artifact is that flares in mirrors or portals don't dim properly
when occluded by something in the main view, and portal flares that should
extend past the portal edge will be overwritten.
==================
*/
void RB_RenderFlares(void)
{
	flare_t	*f;
	qboolean draw = qfalse;

	if( (backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
        return;		// leilei - don't draw flares in the UI. this prevents
	// a very very very very nasty error relating to the trace checks leading to recursive startup in te OA3 menu

	if(r_flareCoeff->modified)
    {
		if(r_flareCoeff->value == 0.0f)
		    flareCoeff = atof(FLARE_STDCOEFF);
	    else
		    flareCoeff = r_flareCoeff->value;
        
		r_flareCoeff->modified = qfalse;
	}

	// Reset currentEntity to world so that any previously referenced entities
	// don't have influence on the rendering of these flares (i.e. RF_ renderer flags).
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.or = backEnd.viewParms.world;
	
	// perform z buffer readback on each flare in this view
	flare_t** prev = &r_activeFlares;
	while ( ( f = *prev ) != NULL )
    {
		// throw out any flares that weren't added last frame
		if( f->addedFrame < backEnd.viewParms.frameCount - 1 )
        {
			*prev = f->next;
			f->next = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;
		if ( (f->frameSceneNum == backEnd.viewParms.frameSceneNum) && (f->inPortal == backEnd.viewParms.isPortal) )
        {
			if (r_flareQuality->integer == 4)		// high flare quality - frequent readpixels, trace
				RB_TestFlare(f);
			else if (r_flareQuality->integer == 3)		// medium flare quality - delayed readpixels, no trace (faster for some cpus than 2)
				RB_TestFlareFast( f, 0 );
			else if (r_flareQuality->integer == 2)		// low flare quality - delayed readpixels, trace
				RB_TestFlareFast( f, 1 );
			else
				RB_TestFlareFast( f, 1 );		// lowest is actually a different surface flare defined elsewhere

			if ( f->drawIntensity )
            {
				draw = qtrue;
			}
			else
            {
				// this flare has completely faded out, so remove it from the chain
				*prev = f->next;
				f->next = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		prev = &f->next;
	}

	if ( !draw ) {
		return;		// none visible
	}

	if ( backEnd.viewParms.isPortal )
		qglDisable (GL_CLIP_PLANE0);


	qglPushMatrix();
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( backEnd.viewParms.viewportX, backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	          backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999 );

	for( f = r_activeFlares ; f ; f = f->next )
    {
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal && f->drawIntensity )
        {
			RB_RenderFlare( f );
		}
	}

	qglPopMatrix();
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix();
}


/*
void RB_DrawSunFlare( void )
{
	vec3_t origin, vec1, vec2;

	if ( !backEnd.skyRenderedThisView ) {
		return;
	}


	if ( backEnd.doneSunFlare)	// leilei - only do sun once
		return;


	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	qglTranslatef (backEnd.viewParms.or.origin[0], backEnd.viewParms.or.origin[1], backEnd.viewParms.or.origin[2]);

	float dist = backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	float size = dist * 0.4;

	VectorScale( tr.sunDirection, dist, origin );
	VectorPerp( tr.sunDirection,vec1 );
	CrossProduct( tr.sunDirection, vec1, vec2 );

	VectorScale( vec1, size, vec1 );
	VectorScale( vec2, size, vec2 );

	// farthest depth range
	qglDepthRange( 1.0, 1.0 );


    vec3_t	coll;
    vec3_t	sunorg;		// sun flare hack
    vec3_t	temp;
    
    coll[0]=tr.sunLight[0]/64;
    coll[1]=tr.sunLight[1]/64;
    coll[2]=tr.sunLight[2]/64;

    int g;
    for (g=0; g<3; g++)
        if (coll[g] > 1)
            coll[g] = 1;
    
    VectorCopy( origin, temp );
    VectorSubtract( temp, vec1, temp );
    VectorAdd( temp, vec2, temp );

    VectorCopy( origin, sunorg );
    VectorSubtract( sunorg, backEnd.viewParms.or.origin, sunorg );
    VectorCopy( backEnd.viewParms.or.origin, sunorg );
    VectorAdd( origin, sunorg, sunorg );

    size = coll[0] + coll[1] + coll[2] * 805;

    RB_AddFlare( (void *)NULL, 0, sunorg, coll, NULL, size, 1.0f, 2);

	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );

	backEnd.doneSunFlare = qtrue;
}
*/

// Init and Flares
void R_InitFlares( void )
{
	int	i;

	memset( r_flareStructs, 0, sizeof( r_flareStructs ) );
	r_activeFlares = NULL;
	r_inactiveFlares = NULL;

	for ( i = 0 ; i < MAX_FLARES ; i++ )
    {
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[i];
	}


	r_flares = ri.Cvar_Get("r_flares", "0", CVAR_ARCHIVE);

    // SetFlareCoeff
    r_flareCoeff = ri.Cvar_Get("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);
    flareCoeff = atof(FLARE_STDCOEFF);

    r_flareMethod = ri.Cvar_Get( "r_flareMethod", "0" , CVAR_ARCHIVE);
    // use high flares for default (lower settings stutter vsync or clip too badly)
    r_flareQuality = ri.Cvar_Get( "r_flareQuality", "4" , CVAR_ARCHIVE);

    r_flareSize = ri.Cvar_Get("r_flareSize", "40", CVAR_CHEAT);
    r_flareFade = ri.Cvar_Get("r_flareFade", "7", CVAR_CHEAT);
 }

