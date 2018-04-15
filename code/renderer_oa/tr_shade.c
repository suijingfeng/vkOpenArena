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

/* 
 * tr_shade.c
 *
 * THIS ENTIRE FILE IS BACK END
 *
 * This file deals with applying shaders to surface data in the tess struct.
 */

#include "tr_local.h"


extern backEndState_t backEnd;
extern trGlobals_t tr;
extern refimport_t ri;


extern shaderCommands_t tess;
extern cvar_t* r_lightmap;



#ifdef USE_RENDERER_DLOPEN
static cvar_t* com_altivec;
#endif

static cvar_t* r_shownormals;
static cvar_t* r_showtris;					// enables wireframe rendering of the world
static cvar_t* r_dlightBacks;
static cvar_t* r_offsetFactor;
static cvar_t* r_offsetUnits;

static qboolean	setArraysOnce;
/*
================
R_ArrayElementDiscrete

This is just for OpenGL conformance testing, it should never be the fastest
================
extern glstate_t glState;

static void APIENTRY R_ArrayElementDiscrete( GLint index )
{
	qglColor4ubv( tess.svars.colors[ index ] );
	if ( glState.currenttmu )
    {
		qglMultiTexCoord2fARB( 0, tess.svars.texcoords[ 0 ][ index ][0], tess.svars.texcoords[ 0 ][ index ][1] );
		qglMultiTexCoord2fARB( 1, tess.svars.texcoords[ 1 ][ index ][0], tess.svars.texcoords[ 1 ][ index ][1] );
	}
    else
    {
		qglTexCoord2fv( tess.svars.texcoords[ 0 ][ index ] );
	}
	qglVertex3fv( tess.xyz[ index ] );
}



static void R_DrawStripElements(int numIndexes, const unsigned int *indexes, void ( APIENTRY *element )(GLint) )
{
    static int c_vertexes;		// for seeing how long our average strips are
    static int c_begins;


	int last[3] = { -1, -1, -1 };
	qboolean even = qfalse;

	c_begins++;

	if ( numIndexes <= 0 ) {
		return;
	}

	qglBegin( GL_TRIANGLE_STRIP );

	// prime the strip
	element( indexes[0] );
	element( indexes[1] );
	element( indexes[2] );
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];
	int i;
	for ( i = 3; i < numIndexes; i += 3 )
	{
		// odd numbered triangle in potential strip
		if( !even )
		{
			// check previous triangle to see if we're continuing a strip,
            // otherwise we're done with this strip so finish it and start a new one
			if( ( indexes[i] == last[2] ) && ( indexes[i+1] == last[1] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;
				assert( indexes[i+2] < tess.numVertexes );
				even = qtrue;
			}
			else
			{
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );

				c_vertexes += 3;

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
            // otherwise we're done with this strip so finish it and start a new one

			if ( ( last[2] == indexes[i+1] ) && ( last[0] == indexes[i+0] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;

				even = qfalse;
			}
			else
			{
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i+0];
		last[1] = indexes[i+1];
		last[2] = indexes[i+2];
	}

	qglEnd();
}
*/

/*
=============================================================

SURFACE SHADERS

=============================================================
*/
static void R_BindAnimatedImage( textureBundle_t *bundle )
{
	if ( bundle->isVideoMap )
    {
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if ( bundle->numImageAnimations <= 1 )
    {
		GL_Bind( bundle->image[0] );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	int64_t index = tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE;
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 )
    {
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_Bind( bundle->image[ index ] );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris(shaderCommands_t *input)
{
	GL_Bind( tr.whiteImage );
	qglColor3f (1,1,1);

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglDepthRange( 0, 0 );

	qglDisableClientState (GL_COLOR_ARRAY);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);

	qglVertexPointer (3, GL_FLOAT, 16, input->xyz);	// padded for SIMD
/*  
	if (qglLockArraysEXT) {
		qglLockArraysEXT(0, input->numVertexes);
	}
*/        
    qglDrawElements( GL_TRIANGLES, input->numIndexes, GL_UNSIGNED_INT, input->indexes );
	
	qglDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals(shaderCommands_t *input)
{
	int		i;


	GL_Bind( tr.whiteImage );
	qglColor3f (1,1,1);
	qglDepthRange( 0, 0 );	// never occluded
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	qglBegin (GL_LINES);
	for (i = 0 ; i < input->numVertexes ; i++)
    {
		qglVertex3fv (input->xyz[i]);
        vec3_t temp;
		VectorMA(input->xyz[i], 2, input->normal[i], temp);
		qglVertex3fv(temp);
	}
	qglEnd();

	qglDepthRange( 0, 1 );
}


/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static void DrawMultitextured( shaderCommands_t *input, int stage )
{
	shaderStage_t *pStage = tess.xstages[stage];

	GL_State( pStage->stateBits );

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
	if ( backEnd.viewParms.isPortal ) {
		qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	//
	// base
	//
	GL_SelectTexture( 0 );
	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	R_BindAnimatedImage( &pStage->bundle[0] );

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( r_lightmap->integer )
		GL_TexEnv( GL_REPLACE );
    else
		GL_TexEnv( tess.shader->multitextureEnv );


	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );

	R_BindAnimatedImage( &pStage->bundle[1] );
        
    qglDrawElements( GL_TRIANGLES, input->numIndexes, GL_UNSIGNED_INT, input->indexes );
	
    //
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisable( GL_TEXTURE_2D );

	GL_SelectTexture( 0 );
}



// Perform dynamic lighting with another rendering pass
static void ProjectDlightTexture( void )
{
	int		i, l;
	unsigned char clipBits[SHADER_MAX_VERTEXES];
	float	texCoordsArray[SHADER_MAX_VERTEXES][2];
	unsigned char colorArray[SHADER_MAX_VERTEXES][4];
	unsigned	hitIndexes[SHADER_MAX_INDEXES];
	
	if ( !backEnd.refdef.num_dlights ) {
		return;
	}

	for( l = 0 ; l < backEnd.refdef.num_dlights ; l++ )
    {
		if ( !( tess.dlightBits & ( 1 << l ) ) )
        {
			continue;	// this surface definately doesn't have any of this light
		}
        

		dlight_t* dl = &backEnd.refdef.dlights[l];

        
		for ( i = 0 ; i < tess.numVertexes ; i++)
        {
			int	clip = 0;
            float modulate = 0;
			
            vec3_t dist;
			
			VectorSubtract( dl->transformed, tess.xyz[i], dist );

			backEnd.pc.c_dlightVertexes++;

			if( !r_dlightBacks->integer && ( dist[0] * tess.normal[i][0] + dist[1] * tess.normal[i][1] + dist[2] * tess.normal[i][2] ) < 0.0f )
            {
				clip = 63;
			}
            else
            {
                texCoordsArray[i][0] = 0.5f + dist[0] / dl->radius;
			    texCoordsArray[i][1] = 0.5f + dist[1] / dl->radius;

				if ( texCoordsArray[i][0] < 0.0f )
					clip |= 1;
                else if ( texCoordsArray[i][0] > 1.0f )
					clip |= 2;

				if ( texCoordsArray[i][1] < 0.0f )
					clip |= 4;
				else if ( texCoordsArray[i][1] > 1.0f )
					clip |= 8;


				// modulate the strength based on the height and color

				if ( dist[2] > dl->radius )
                {
					clip |= 16;
					modulate = 0.0f;
				}
                else if( dist[2] < -dl->radius )
                {
					clip |= 32;
					modulate = 0.0f;
				}
                else
                {
					dist[2] = fabs(dist[2]);
					if ( dist[2] < dl->radius * 0.5f )
						modulate = 1.0f;
                    else
						modulate = 2.0f * (dl->radius - dist[2]) / dl->radius;
				}
			}

			clipBits[i] = clip;

			colorArray[i][0] = dl->color[0] * 255.0f * modulate;
			colorArray[i][1] = dl->color[1] * 255.0f * modulate;
			colorArray[i][2] = dl->color[2] * 255.0f * modulate;
			colorArray[i][3] = 255;
		}

        
		// build a list of triangles that need light
    
        int	numIndexes = 0;
		for( i = 0; i < tess.numIndexes; )
        {
			int a = tess.indexes[i++];
			int b = tess.indexes[i++];
			int c = tess.indexes[i++];
			if ( clipBits[a] & clipBits[b] & clipBits[c] )
				continue;	// not lighted

			hitIndexes[numIndexes++] = a;
			hitIndexes[numIndexes++] = b;
			hitIndexes[numIndexes++] = c;
		}

		if( !numIndexes )
			continue;


		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

		GL_Bind( tr.dlightImage );
		
        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light where they aren't rendered
		if ( dl->additive )
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		else
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );


        qglDrawElements( GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, hitIndexes );
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}
}



/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void )
{
	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );

	fog_t* fog = tr.world->fogs + tess.fogNum;
    int i;
	for ( i = 0; i < tess.numVertexes; i++ ) {
		* ( int * )&tess.svars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[0] );

	GL_Bind( tr.fogImage );

	if ( tess.shader->fogPass == FP_EQUAL )
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	else
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );


	// leilei -  hmm., will integrate additive fog in the future
	//
	//	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE );
    qglDrawElements( GL_TRIANGLES, tess.numIndexes, GL_UNSIGNED_INT, tess.indexes );
}


// leilei - reveal normals to GLSL for light processing. HACK HACK HACK HACK HACK HACK
static void RB_CalcNormal( unsigned char *colors )
{
	int			i;
	float		*v = tess.xyz[0];
	float		*normal = ( float * ) tess.normal; 
	vec3_t			n, m;

	int numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4)
    {
		int y;
		for (y=0;y<3;y++)
        {
			n[y] = normal[y];
        }

		float mid = n[1] + n[2];
		
        if (mid < 0)
            mid *= -1;
			
        m[0] = 127 + (n[0]*128);
        m[1] = 127 + (n[1]*128);
        m[2] = 127 + (n[2]*128);
		
		colors[i*4+0] = m[0];
		colors[i*4+1] = m[1];
		colors[i*4+2] = m[2];
		colors[i*4+3] = 255;
	}
}

/*
** RB_CalcUniformColor
**
** RiO; Uniform vertex color lighting for cel shading
*/


static void RB_CalcUniformColor( unsigned char *colors )
{
	vec4_t			uniformLight;

	trRefEntity_t* ent = backEnd.currentEntity;

	//VectorCopy( ent->directedLight, directedLight );

	VectorAdd( ent->ambientLight, ent->ambientLight, uniformLight );

	float normalize = NormalizeColor( uniformLight, uniformLight );
	if ( normalize > 255 )
        normalize = 255;

	VectorScale( uniformLight, normalize, uniformLight );
	uniformLight[3] = 255;

	int numVertexes = tess.numVertexes;

    int i;
	for (i = 0 ; i < numVertexes ; i++ )
    {
		colors[i*4+0] = uniformLight[0];
		colors[i*4+1] = uniformLight[1];
		colors[i*4+2] = uniformLight[2];
		colors[i*4+3] = uniformLight[3];
	}
}



static void RB_CalcDiffuseColor( unsigned char *colors )
{
	int	i;
	trRefEntity_t* ent = backEnd.currentEntity;

    int numVertexes = tess.numVertexes;

	for (i = 0 ; i < numVertexes ; i++)
    {
		float incoming = DotProduct (tess.normal[i], ent->lightDir);
		
        if ( incoming <= 0 )
        {
			*(int *)&colors[i*4] = ent->ambientLightInt;
			continue;
		} 
		int j = ent->ambientLight[0] + incoming * ent->directedLight[0];
		if ( j > 255 )
			colors[i*4+0] = 255;
        else
            colors[i*4+0] = j;


		j = ent->ambientLight[1] + incoming * ent->directedLight[1];
		if ( j > 255 )
			colors[i*4+1] = 255;
        else
		    colors[i*4+1] = j;

		j = ent->ambientLight[2] + incoming * ent->directedLight[2];
		if ( j > 255 )
			colors[i*4+2] = 255;
        else
		    colors[i*4+2] = j;

		colors[i*4+3] = 255;
	}
}

static void ComputeColors( shaderStage_t *pStage )
{
	int	i;

	// rgbGen
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
			break;
		case CGEN_LIGHTING_DIFFUSE:
			if (r_shownormals->integer > 1)
            {
                // leilei - debug normals, or use the normals as a color for a lighting shader
				RB_CalcNormal( ( unsigned char * ) tess.svars.colors );
                break;
			}
			RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_LIGHTING_UNIFORM:
			RB_CalcUniformColor( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_LIGHTING_DYNAMIC:
			RB_CalcDynamicColor( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_LIGHTING_FLAT_AMBIENT:
			RB_CalcFlatAmbient( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_LIGHTING_FLAT_DIRECT:
			RB_CalcFlatDirect( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_EXACT_VERTEX:
			memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			break;
		case CGEN_CONST:
			for ( i = 0; i < tess.numVertexes; i++ )
            {
				*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
			}
			break;
		case CGEN_VERTEX:
			if ( tr.identityLight == 1 )
			{
				memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
					tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
					tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
		case CGEN_VERTEX_LIT:		// leilei - mixing vertex colors with lighting through a glorious light hack
			{			// 	    should only be used for entity models, not map assets!
			vec3_t	dcolor, acolor;	// to save the color from actual light
			vec3_t	vcolor;
			int y;


			// Backup our colors
			VectorCopy( backEnd.currentEntity->ambientLight, acolor );			
			VectorCopy( backEnd.currentEntity->directedLight, dcolor );			
			VectorCopy( backEnd.currentEntity->e.shaderRGBA, vcolor );			

			// Make our vertex color take over 

			for(y=0;y<3;y++)
            {
				backEnd.currentEntity->ambientLight[y] 	*= (vcolor[y] / 255);

				if (backEnd.currentEntity->ambientLight[y] < 1)   
                    backEnd.currentEntity->ambientLight[y] = 1; // black!!!
                else if (backEnd.currentEntity->ambientLight[y] > 255)
                    backEnd.currentEntity->ambientLight[y] = 255; // white!!!!!
			    //	backEnd.currentEntity->ambientLight[y] 	*= (vcolor[y] / 255);
			    //	backEnd.currentEntity->directedLight[y] *= (vcolor[y] / 255);
			}
		
			// run it through our favorite preferred lighting calculation functions
			RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );

			// Restore light color for any other stage that doesn't do it
			VectorCopy( acolor, backEnd.currentEntity->ambientLight);			
			VectorCopy( dcolor, backEnd.currentEntity->directedLight);			
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			if ( tr.identityLight == 1 )
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
					tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
					tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
				}
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
					tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
					tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
				}
			}
			break;
		case CGEN_FOG:
			{
				fog_t* fog = tr.world->fogs + tess.fogNum;

				for( i = 0; i < tess.numVertexes; i++ )
                {
					*( int * )&tess.svars.colors[i] = fog->colorInt;
				}
			}
			break;
		case CGEN_WAVEFORM:
			RB_CalcWaveColor( &pStage->rgbWave, (unsigned char *)tess.svars.colors );
			break;
		case CGEN_ENTITY:
			RB_CalcColorFromEntity(( unsigned char *)tess.svars.colors );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity( (unsigned char *)tess.svars.colors );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	    case AGEN_SKIP:	break;
	
        case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( i = 0; i < tess.numVertexes; i++ ) {
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}break;
        
        case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}break;
        
        case AGEN_WAVEFORM:
		    RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) tess.svars.colors ); break;
        
        case AGEN_LIGHTING_SPECULAR:
			RB_CalcSpecularAlphaNew( ( unsigned char * ) tess.svars.colors ); break;
        
        case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( unsigned char * ) tess.svars.colors );
		break;
        
        case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
		break;
        
        case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}break;
        
        case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; i++ )
        {
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        } break;
	    
        case AGEN_PORTAL:
		{
			unsigned char alpha;

			for ( i = 0; i < tess.numVertexes; i++ )
			{
				vec3_t v;

				VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
				float len = VectorLength( v );
				len /= tess.shader->portalRange;

				if ( len < 0 )
				{
					alpha = 0;
				}
				else if ( len > 1 )
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				tess.svars.colors[i][3] = alpha;
			}
		}break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum )
	{
		switch ( pStage->adjustColorsForFog )
		{
            case ACFF_MODULATE_RGB:
                RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
                break;
            case ACFF_MODULATE_ALPHA:
                RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
                break;
            case ACFF_MODULATE_RGBA:
                RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
                break;
            case ACFF_NONE:
                break;
		}
	}
}


static void ComputeTexCoords( shaderStage_t *pStage )
{
	int	b, i, tm;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ )
    {
		// generate the texture coordinates
		switch ( pStage->bundle[b].tcGen )
		{
            case TCGEN_IDENTITY:
                memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
                break;
            case TCGEN_TEXTURE:
                for ( i = 0 ; i < tess.numVertexes ; i++ )
                {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
                }break;
            case TCGEN_LIGHTMAP:
                for ( i = 0 ; i < tess.numVertexes ; i++ )
                {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
                }
                break;
            case TCGEN_VECTOR:
                for ( i = 0 ; i < tess.numVertexes ; i++ )
                {
                    tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
                    tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
                }
                break;
            case TCGEN_FOG:
                RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_ENVIRONMENT_MAPPED:
                RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] ); 
                break;
            case TCGEN_ENVIRONMENT_MAPPED_WATER:
                RB_CalcEnvironmentTexCoordsJO( ( float * ) tess.svars.texcoords[b] ); 			
                break;
            case TCGEN_ENVIRONMENT_CELSHADE_MAPPED:
                RB_CalcEnvironmentCelShadeTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_ENVIRONMENT_CELSHADE_LEILEI:
                RB_CalcCelTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_BAD:
                return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ ) 
        {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
                case TMOD_NONE:
                    tm = TR_MAX_TEXMODS;		// break out of for loop
                    break;

                case TMOD_TURBULENT:
                    RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
                    break;

                case TMOD_ENTITY_TRANSLATE:
                    RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord, ( float * ) tess.svars.texcoords[b] );
                    break;

                case TMOD_SCROLL:
                    RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll, ( float * ) tess.svars.texcoords[b] );
                    break;

                case TMOD_SCALE:
                    RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale, ( float * ) tess.svars.texcoords[b] );
                    break;
                
                case TMOD_STRETCH:
                    RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
                    break;
/*
                case TMOD_ATLAS:
                    RB_CalcAtlasTexCoords(  &pStage->bundle[b].texMods[tm].atlas, ( float * ) tess.svars.texcoords[b] );
                    break;
*/
                case TMOD_LIGHTSCALE:
                    RB_CalcLightscaleTexCoords( ( float * ) tess.svars.texcoords[b] );
                    break;

                case TMOD_TRANSFORM:
                    RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm], ( float * ) tess.svars.texcoords[b] );
                    break;

                case TMOD_ROTATE:
                    RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed, ( float * ) tess.svars.texcoords[b] );
                    break;

                default:
                    ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", pStage->bundle[b].texMods[tm].type, tess.shader->name );
                    break;
			}
		}
	}
}



static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
	int stage;

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if (!pStage )
		{
			break;
		}

		ComputeColors( pStage );
		ComputeTexCoords( pStage );

		if ( !setArraysOnce )
		{
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
		}


		//
		// do multitexture
		//
		if ( pStage->bundle[1].image[0] != 0 )
		{
			DrawMultitextured( input, stage );
		}
		else
		{
			if ( !setArraysOnce )
			{
				qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
			}

			// set state
			R_BindAnimatedImage( &pStage->bundle[0] );

			GL_State( pStage->stateBits );

			// draw
            qglDrawElements( GL_TRIANGLES, input->numIndexes, GL_UNSIGNED_INT, input->indexes );
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap ) )
		{
			break;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum )
{
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if(tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime)
    {
		tess.shaderTime = tess.shader->clampTime;
	}
}



/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t* input = &tess;
	shader_t* shader = input->shader;

	RB_DeformTessGeometry();

	// set face culling appropriately
	GL_Cull( shader->cullType );

	// set polygon offset if necessary
	if( shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	// if there is only a single pass then we can 
    // enable color and texture arrays before we compile, 
    // otherwise we need to avoid compiling those arrays 
    // since they will change during multipass rendering
	
    if( tess.numPasses > 1 || shader->multitextureEnv )
	{
		setArraysOnce = qfalse;
		qglDisableClientState(GL_COLOR_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		setArraysOnce = qtrue;

		qglEnableClientState( GL_COLOR_ARRAY);
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );
	}

    
	// lock XYZ
	qglVertexPointer (3, GL_FLOAT, 16, input->xyz);	// padded for SIMD
/*
    if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
	}
*/

	// enable color and texcoord arrays after the lock if necessary
	if ( !setArraysOnce )
	{
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglEnableClientState( GL_COLOR_ARRAY );
	}

	// call shader function
	RB_IterateStagesGeneric( input );

	// now do any dynamic lighting needed
	if( tess.dlightBits && (tess.shader->sort <= SS_OPAQUE) && !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) )
    {
		ProjectDlightTexture();
    }


	// now do fog
	if ( tess.fogNum && tess.shader->fogPass )
    {
		RB_FogPass();
	}

	// reset polygon offset
	if ( shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}


/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void )
{
	shaderCommands_t *input = &tess;
	shader_t *shader = input->shader;


	// compute colors
	RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );


	// set face culling appropriately
	GL_Cull( shader->cullType );


	// set arrays and lock
	qglEnableClientState( GL_COLOR_ARRAY);
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );
	qglVertexPointer (3, GL_FLOAT, 16, input->xyz);
/*
	if ( qglLockArraysEXT )
	{
		qglLockArraysEXT(0, input->numVertexes);
	}
*/

	// call special shade routine
	R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
	GL_State( tess.xstages[0]->stateBits );
    
    // draw
    qglDrawElements( GL_TRIANGLES, input->numIndexes, GL_UNSIGNED_INT, input->indexes );

	// now do any dynamic lighting needed
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE )
    {
		ProjectDlightTexture();
	}

	// now do fog
	if( tess.fogNum && tess.shader->fogPass )
    {
		RB_FogPass();
	}

}



void RB_StageIteratorLightmappedMultitexture( void )
{
	shaderCommands_t* input = &tess;
	shader_t* shader = input->shader;


	// set face culling appropriately
	GL_Cull( shader->cullType );

	// set color, pointers, and lock
	GL_State( GLS_DEFAULT );
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.constantColor255 );

	// select base stage
	GL_SelectTexture( 0 );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );


	// configure second stage
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	if ( r_lightmap->integer )
    {
		GL_TexEnv( GL_REPLACE );
	}
    else
    {
		GL_TexEnv( GL_MODULATE );
	}
	R_BindAnimatedImage( &tess.xstages[0]->bundle[1] );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][1] );
/*
	// lock arrays
	if ( qglLockArraysEXT )
    {
		qglLockArraysEXT(0, input->numVertexes);
	}
*/
    // draw
    qglDrawElements( GL_TRIANGLES, input->numIndexes, GL_UNSIGNED_INT, input->indexes );

	// disable texturing on TEXTURE1, then select TEXTURE0
	qglDisable( GL_TEXTURE_2D );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	GL_SelectTexture( 0 );


	// now do any dynamic lighting needed
	if ( tess.dlightBits && (tess.shader->sort <= SS_OPAQUE) )
    {
		ProjectDlightTexture();
	}

	// now do fog
	if ( tess.fogNum && tess.shader->fogPass )
    {
		RB_FogPass();
	}
}



void RB_EndSurface( void )
{
	shaderCommands_t* input = &tess;

	if (input->numIndexes == 0)
		return;


	if (input->indexes[SHADER_MAX_INDEXES-1] != 0)
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");

	if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0)
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");


	if ( tess.shader == tr.shadowShader )
    {
		RB_ShadowTessEnd();
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	// call off to shader specific tess end function
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer )
		DrawTris(input);

	if ( r_shownormals->integer )
		DrawNormals(input);

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
}


void R_InitShade(void)
{
    #ifdef USE_RENDERER_DLOPEN
	    com_altivec = ri.Cvar_Get("com_altivec", "0", CVAR_ARCHIVE);
    #endif

	r_showtris = ri.Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
    
    // draws wireframe normals
    r_shownormals = ri.Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);

    r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );

//    r_primitives = ri.Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

	r_offsetFactor = ri.Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri.Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );

    // rendering primitives, default is to use triangles if compiled vertex arrays are present
//	ri.Printf( PRINT_ALL, "rendering primitives: ");

    // "0" = based on compiled vertex array existance
	// "1" = glDrawElemet tristrips
	// "2" = glDrawElements triangles
	// "-1" = no drawing
/*
	int primitives = r_primitives->integer;
    if ( primitives == 0 )
    {
        if ( qglLockArraysEXT )
        {
            primitives = 2;
        }
        else {
            primitives = 1;
        }
    }

    if ( primitives == -1 ) {
        ri.Printf( PRINT_ALL, "none\n" );
    }
    else if ( primitives == 2 ) {
        ri.Printf( PRINT_ALL, "single glDrawElements\n" );
    }
    else if ( primitives == 1 ) {
        ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
    }
    else if ( primitives == 3 ) {
        ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
    }
*/
}
