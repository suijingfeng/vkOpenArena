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
// tr_models.c -- model loading and caching

#include "tr_local.h"


typedef struct
{
	char *ext;
	qhandle_t (*ModelLoader)( const char *, model_t * );
} modelExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple models of different formats available
static qboolean R_LoadMD3(model_t *mod, int lod, void *buffer, const char* modName);
static qboolean R_LoadMDR(model_t *mod, void *buffer, int filesize, const char *name );
static qhandle_t R_RegisterMD3(const char *name, model_t *mod);
static qhandle_t R_RegisterMDR(const char *name, model_t *mod);
static qhandle_t R_RegisterIQM(const char *name, model_t *mod);

static const modelExtToLoaderMap_t modelLoaders[ ] =
{
	{ "iqm", R_RegisterIQM },
	{ "mdr", R_RegisterMDR },
	{ "md3", R_RegisterMD3 }
};
static const int numModelLoaders = ARRAY_LEN(modelLoaders);



static qhandle_t R_RegisterMD3(const char *name, model_t *mod)
{
	
	char* buf;
	int			lod;
	qboolean	loaded = qfalse;
	int			numLoaded;
	char filename[MAX_QPATH], namebuf[MAX_QPATH+20];
	char *fext, defex[] = "md3";

	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');
	if(!fext)
		fext = defex;
	else
	{
		*fext = '\0';
		fext++;
	}

	for (lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod--)
	{
		if(lod)
			snprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			snprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

		ri.FS_ReadFile( namebuf, &buf );
		if(!buf)
			continue;
#if defined( Q3_BIG_ENDIAN )		
		int ident = LittleLong(*(int *)buf);
#else
		int ident = *(int *)buf;
#endif		
		if (ident == MD3_IDENT)
			loaded = R_LoadMD3(mod, lod, buf, name);
		else
			ri.Printf(PRINT_WARNING,"R_RegisterMD3: unknown fileid for %s\n", name);
		
		ri.FS_FreeFile(buf);

		if(loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
		else
			break;
	}

	if(numLoaded)
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for(lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod + 1];
		}

		return mod->index;
	}

	ri.Printf(PRINT_WARNING,"R_RegisterMD3: couldn't load %s\n", name);

	mod->type = MOD_BAD;
	return 0;
}


static qhandle_t R_RegisterMDR(const char *name, model_t *mod)
{
	char* buf;
	qboolean loaded = qfalse;
	int filesize;

	filesize = ri.FS_ReadFile(name, &buf);
	if(!buf)
	{
		mod->type = MOD_BAD;
		return 0;
	}
	
#if defined( Q3_BIG_ENDIAN )		
	int ident = LittleLong(*(int *)buf);
#else
	int ident = *(int *)buf;
#endif	
	if(ident == MDR_IDENT)
		loaded = R_LoadMDR(mod, buf, filesize, name);

	ri.FS_FreeFile (buf);
	
	if(!loaded)
	{
		ri.Printf(PRINT_WARNING,"R_RegisterMDR: couldn't load mdr file %s\n", name);
		mod->type = MOD_BAD;
		return 0;
	}
	
	return mod->index;
}


static qhandle_t R_RegisterIQM(const char *name, model_t *mod)
{	
	char* buf;
	qboolean loaded = qfalse;
	int filesize = ri.FS_ReadFile(name, &buf);
	if(!buf)
	{
		mod->type = MOD_BAD;
		return 0;
	}
	
	loaded = R_LoadIQM(mod, buf, filesize, name);

	ri.FS_FreeFile(buf);
	
	if(!loaded)
	{
		ri.Printf(PRINT_WARNING,"R_RegisterIQM: couldn't load iqm file %s\n", name);
		mod->type = MOD_BAD;
		return 0;
	}
	
	return mod->index;
}


static qhandle_t RE_RegisterModelReal( const char *name )
{
	model_t		*mod;
	qhandle_t	hModel;
	//qboolean	orgNameFailed = qfalse;
	int			orgLoader = -1;
	int			i;
	char		localName[ MAX_QPATH ];
	const char	*ext;
	char		altName[ MAX_QPATH ];

	if ( !name || !name[0] ) {
		ri.Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	//
	// search the currently loaded models
	//
	for ( hModel = 1 ; hModel < tr.numModels; hModel++ ) {
		mod = tr.models[hModel];
		if ( !strcmp( mod->name, name ) ) {
			if( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL )
    {
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	R_IssuePendingRenderCommands();

	mod->type = MOD_BAD;
	mod->numLods = 0;

	//
	// load the files
	//
	Q_strncpyz( localName, name, MAX_QPATH );

	ext = COM_GetExtension( localName );

	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numModelLoaders; i++ )
		{
			if( !Q_stricmp( ext, modelLoaders[ i ].ext ) )
			{
				// Load
				hModel = modelLoaders[ i ].ModelLoader( localName, mod );
				break;
			}
		}

		// A loader was found
		if( i < numModelLoaders )
		{
			if( !hModel )
			{
				// Loader failed, most likely because the file isn't there;
				// try again without the extension
				//orgNameFailed = qtrue;
				orgLoader = i;
				R_StripExtension(name, localName, MAX_QPATH);
			}
			else
			{
				// Something loaded
				return mod->index;
			}
		}
	}

	// Try and find a suitable match using all
	// the model formats supported
	for( i = 0; i < numModelLoaders; i++ )
	{
		if (i == orgLoader)
			continue;

		snprintf( altName, sizeof (altName), "%s.%s", localName, modelLoaders[ i ].ext );

		// Load
		hModel = modelLoaders[ i ].ModelLoader( altName, mod );

		if( hModel )
		{
            /*
			if( orgNameFailed )
			{
				ri.Printf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n", name, altName );
			}
            */
			break;
		}
	}

	return hModel;
}


static qboolean R_LoadMD3(model_t* mod, int lod, void *buffer, const char* modName)
{
	int	i, j;

    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;

	md3Header_t* pinmodel = (md3Header_t *)buffer;

	int version = LittleLong(pinmodel->version);
	if(version != MD3_VERSION)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n", modName, version, MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MESH;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mod->md3[lod] = ri.Hunk_Alloc(size, h_low);
    pinmodel->ofsEnd = LittleLong(pinmodel->ofsEnd);
	memcpy(mod->md3[lod], buffer, pinmodel->ofsEnd);

    mod->md3[lod]->ident = LittleLong(mod->md3[lod]->ident);
    mod->md3[lod]->version = LittleLong(mod->md3[lod]->version);
    mod->md3[lod]->numFrames = LittleLong(mod->md3[lod]->numFrames);
    mod->md3[lod]->numTags = LittleLong(mod->md3[lod]->numTags);
    mod->md3[lod]->numSurfaces = LittleLong(mod->md3[lod]->numSurfaces);
    mod->md3[lod]->ofsFrames = LittleLong(mod->md3[lod]->ofsFrames);
    mod->md3[lod]->ofsTags = LittleLong(mod->md3[lod]->ofsTags);
    mod->md3[lod]->ofsSurfaces = LittleLong(mod->md3[lod]->ofsSurfaces);
    mod->md3[lod]->ofsEnd = LittleLong(mod->md3[lod]->ofsEnd);

	if( mod->md3[lod]->numFrames < 1 )
    {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", modName);
		return qfalse;
	}

	// swap all the frames
    frame = (md3Frame_t *) ( (unsigned char *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++)
    {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ )
		{
            frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
            frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
	    	frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (unsigned char *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++)
    {
        surf->ident = LittleLong(surf->ident);
        surf->flags = LittleLong(surf->flags);
        surf->numFrames = LittleLong(surf->numFrames);
        surf->numShaders = LittleLong(surf->numShaders);
        surf->numTriangles = LittleLong(surf->numTriangles);
        surf->ofsTriangles = LittleLong(surf->ofsTriangles);
        surf->numVerts = LittleLong(surf->numVerts);
        surf->ofsShaders = LittleLong(surf->ofsShaders);
        surf->ofsSt = LittleLong(surf->ofsSt);
        surf->ofsXyzNormals = LittleLong(surf->ofsXyzNormals);
        surf->ofsEnd = LittleLong(surf->ofsEnd);
		
		if ( surf->numVerts >= SHADER_MAX_VERTEXES )
        {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on %s (%i).\n",
				modName, SHADER_MAX_VERTEXES - 1, surf->name[0] ? surf->name : "a surface", surf->numVerts );
			return qfalse;
		}
		if ( surf->numTriangles*3 >= SHADER_MAX_INDEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i triangles on %s (%i).\n",
				modName, ( SHADER_MAX_INDEXES / 3 ) - 1, surf->name[0] ? surf->name : "a surface",	surf->numTriangles );
			return qfalse;
		}
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen(surf->name);
		if((j > 2) && (surf->name[j-2] == '_') )
		{
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for(j = 0; j < surf->numShaders; j++, shader++)
        {
            shader_t* sh = R_FindShader(shader->name, LIGHTMAP_NONE, qtrue);
			if(sh->defaultShader)
			{
				shader->shaderIndex = 0;
			}
			else
			{
				shader->shaderIndex = sh->index;
			}
        }

		// swap all the triangles
		tri = (md3Triangle_t *) ( (unsigned char *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ )
        {
			tri->indexes[0] = LittleLong(tri->indexes[0]);
			tri->indexes[1] = LittleLong(tri->indexes[1]);
			tri->indexes[2] = LittleLong(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (unsigned char *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ )
        {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return qtrue;
}





/*
=============================================================
UNCOMPRESSING BONES
=============================================================
*/


#define MC_BITS_X       (16)
#define MC_BITS_Y       (16)
#define MC_BITS_Z       (16)
#define MC_BITS_VECT    (16)

#define MC_SCALE_X      (1.0f/64)
#define MC_SCALE_Y      (1.0f/64)
#define MC_SCALE_Z      (1.0f/64)

#define MC_MASK_X       ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y       ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z       ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT    ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT   (1.0f/(float)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X        (0)
#define MC_SHIFT_X      (0)

#define MC_POS_Y        ((((MC_BITS_X))/8))
#define MC_SHIFT_Y      ((((MC_BITS_X)%8)))

#define MC_POS_Z        ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z      ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33      ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33    ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))


static void MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val = (int)((unsigned short *)(comp))[0];
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	val=(int)((unsigned short *)(comp))[1];
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	val=(int)((unsigned short *)(comp))[2];
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	val=(int)((unsigned short *)(comp))[3];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[4];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[5];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[6];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[7];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[8];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[9];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[10];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[11];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][2]=((float)(val))*MC_SCALE_VECT;
}



static qboolean R_LoadMDR( model_t *mod, void *buffer, int filesize, const char *mod_name ) 
{
	int					i, j, k, l;
	mdrHeader_t* mdr;
	mdrFrame_t			*frame;
	mdrLOD_t			*lod, *curlod;
	mdrSurface_t		*surf, *cursurf;
	mdrTriangle_t		*tri, *curtri;
	mdrVertex_t			*v, *curv;
	mdrWeight_t			*weight, *curweight;
	mdrTag_t			*tag, *curtag;
	shader_t			*sh;

	mdrHeader_t* pinmodel = (mdrHeader_t *)buffer;

    #if defined( Q3_LITTLE_ENDIAN )

	pinmodel->version = LittleLong(pinmodel->version);

    #endif

	if (pinmodel->version != MDR_VERSION) 
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has wrong version (%i should be %i)\n", mod_name, pinmodel->version, MDR_VERSION);
		return qfalse;
	}

    #if defined( Q3_LITTLE_ENDIAN )
	int size = LittleLong(pinmodel->ofsEnd);
    #else
	int size = pinmodel->ofsEnd;
    #endif

	if(size > filesize)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name);
		return qfalse;
	}
	
	mod->type = MOD_MDR;

	pinmodel->numFrames = LittleLong(pinmodel->numFrames);
	pinmodel->numBones = LittleLong(pinmodel->numBones);
	pinmodel->ofsFrames = LittleLong(pinmodel->ofsFrames);

	// This is a model that uses some type of compressed Bones. We don't want to uncompress every bone for each rendered frame
	// over and over again, we'll uncompress it in this function already, so we must adjust the size of the target mdr.
	if(pinmodel->ofsFrames < 0)
	{
		// mdrFrame_t is larger than mdrCompFrame_t:
		size += pinmodel->numFrames * sizeof(frame->name);
		// now add enough space for the uncompressed bones.
		size += pinmodel->numFrames * pinmodel->numBones * ((sizeof(mdrBone_t) - sizeof(mdrCompBone_t)));
	}
	
	// simple bounds check
	if(pinmodel->numBones < 0 ||
		sizeof(*mdr) + pinmodel->numFrames * (sizeof(*frame) + (pinmodel->numBones - 1) * sizeof(*frame->bones)) > size)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
		return qfalse;
	}

	mod->dataSize += size;
	mod->modelData = mdr = ri.Hunk_Alloc( size, h_low );

	// Copy all the values over from the file and fix endian issues in the process, if necessary.
	
	mdr->ident = LittleLong(pinmodel->ident);
	mdr->version = pinmodel->version;	// Don't need to swap byte order on this one, we already did above.
	Q_strncpyz(mdr->name, pinmodel->name, sizeof(mdr->name));
	mdr->numFrames = pinmodel->numFrames;
	mdr->numBones = pinmodel->numBones;
	mdr->numLODs = LittleLong(pinmodel->numLODs);
	mdr->numTags = LittleLong(pinmodel->numTags);
	// We don't care about the other offset values, we'll generate them ourselves while loading.

	mod->numLods = mdr->numLODs;

	if ( mdr->numFrames < 1 ) 
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has no frames\n", mod_name);
		return qfalse;
	}

	/* The first frame will be put into the first free space after the header */
	frame = (mdrFrame_t *)(mdr + 1);
	mdr->ofsFrames = (int)((byte *) frame - (byte *) mdr);
		
	if (pinmodel->ofsFrames < 0)
	{
				
		// compressed model..
        unsigned char *pTem = ((unsigned char *) pinmodel - pinmodel->ofsFrames);
		mdrCompFrame_t* cframe = (mdrCompFrame_t *)pTem;
		
		for(i = 0; i < mdr->numFrames; i++)
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(cframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(cframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(cframe->localOrigin[j]);
			}

			frame->radius = LittleFloat(cframe->radius);
			frame->name[0] = '\0';	// No name supplied in the compressed version.
			
			for(j = 0; j < mdr->numBones; j++)
			{
#if defined( Q3_BIG_ENDIAN )
				for(k = 0; k < (sizeof(cframe->bones[j].Comp) / 2); k++)
				{
					// Do swapping for the uncompressing functions. They seem to use shorts
					// values only, so I assume this will work. Never tested it on other
					// platforms, though.
					
					((unsigned short *)(cframe->bones[j].Comp))[k] =
						LittleShort( ((unsigned short *)(cframe->bones[j].Comp))[k] );
				}
#endif				
				/* Now do the actual uncompressing */
				MC_UnCompress(frame->bones[j].matrix, cframe->bones[j].Comp);
			}
			
            {
			    // Next Frame...
                mdrCompBone_t *p = &(cframe->bones[j]);
			    cframe = (mdrCompFrame_t *) p;
            }

            {
                mdrBone_t *p = &frame->bones[j];
                frame = (mdrFrame_t *) p;
            }
		}
	}
	else
	{
		
		// uncompressed model...
		//
    
		mdrFrame_t* curframe = (mdrFrame_t *)((byte *) pinmodel + pinmodel->ofsFrames);
		
		// swap all the frames
		for ( i = 0 ; i < mdr->numFrames ; i++) 
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(curframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(curframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j]);
			}
			
			frame->radius = LittleFloat(curframe->radius);
			Q_strncpyz(frame->name, curframe->name, sizeof(frame->name));
	
            
#if defined( Q3_BIG_ENDIAN )
            for (j = 0; j < (int) (mdr->numBones * sizeof(mdrBone_t) / 4); j++) 
			{

				((float *)frame->bones)[j] = FloatSwap( &((float *)curframe->bones)[j] );
            }
#else
            for (j = 0; j < mdr->numBones; j++) 
			{
                frame->bones[j] = curframe->bones[j];
            }
#endif		

            {
			    mdrBone_t* p = &curframe->bones[mdr->numBones];
                curframe = (mdrFrame_t *) p;
            }

            {
                mdrBone_t* p = &frame->bones[mdr->numBones];
                frame = (mdrFrame_t *) p;
            }
		}
	}
	
	// frame should now point to the first free address after all frames.
	lod = (mdrLOD_t *) frame;
	mdr->ofsLODs = (int) ((byte *) lod - (byte *)mdr);
	
	curlod = (mdrLOD_t *)((byte *) pinmodel + LittleLong(pinmodel->ofsLODs));
		
	// swap all the LOD's
	for ( l = 0 ; l < mdr->numLODs ; l++)
	{
		// simple bounds check
		if((byte *) (lod + 1) > (byte *) mdr + size)
		{
			ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
			return qfalse;
		}

		lod->numSurfaces = LittleLong(curlod->numSurfaces);
		
		// swap all the surfaces
		surf = (mdrSurface_t *) (lod + 1);
		lod->ofsSurfaces = (int)((byte *) surf - (byte *) lod);
		cursurf = (mdrSurface_t *) ((byte *)curlod + LittleLong(curlod->ofsSurfaces));
		
		for ( i = 0 ; i < lod->numSurfaces ; i++)
		{
			// simple bounds check
			if((byte *) (surf + 1) > (byte *) mdr + size)
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}

			// first do some copying stuff
			
			surf->ident = SF_MDR;
			Q_strncpyz(surf->name, cursurf->name, sizeof(surf->name));
			Q_strncpyz(surf->shader, cursurf->shader, sizeof(surf->shader));
			
			surf->ofsHeader = (byte *) mdr - (byte *) surf;
			
			surf->numVerts = LittleLong(cursurf->numVerts);
			surf->numTriangles = LittleLong(cursurf->numTriangles);
			// numBoneReferences and BoneReferences generally seem to be unused
			
			// now do the checks that may fail.
			if ( surf->numVerts >= SHADER_MAX_VERTEXES ) 
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i verts on %s (%i).\n",
					  mod_name, SHADER_MAX_VERTEXES - 1, surf->name[0] ? surf->name : "a surface",
					  surf->numVerts );
				return qfalse;
			}
			if ( surf->numTriangles*3 >= SHADER_MAX_INDEXES ) 
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i triangles on %s (%i).\n",
					  mod_name, ( SHADER_MAX_INDEXES / 3 ) - 1, surf->name[0] ? surf->name : "a surface",
					  surf->numTriangles );
				return qfalse;
			}
			// lowercase the surface name so skin compares are faster
			Q_strlwr( surf->name );

			// register the shaders
			sh = R_FindShader(surf->shader, LIGHTMAP_NONE, qtrue);
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
			
			// now copy the vertexes.
			v = (mdrVertex_t *) (surf + 1);
			surf->ofsVerts = (int)((byte *) v - (byte *) surf);
			curv = (mdrVertex_t *) ((byte *)cursurf + LittleLong(cursurf->ofsVerts));
			
			for(j = 0; j < surf->numVerts; j++)
			{
				curv->numWeights = LittleLong(curv->numWeights);
			
				// simple bounds check
				if(curv->numWeights < 0 || (unsigned char *) (v + 1) + (curv->numWeights - 1) * sizeof(*weight) > (unsigned char *) mdr + size)
				{
					ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
					return qfalse;
				}

				v->normal[0] = LittleFloat(curv->normal[0]);
				v->normal[1] = LittleFloat(curv->normal[1]);
				v->normal[2] = LittleFloat(curv->normal[2]);
				
				v->texCoords[0] = LittleFloat(curv->texCoords[0]);
				v->texCoords[1] = LittleFloat(curv->texCoords[1]);
				
				v->numWeights = curv->numWeights;
				weight = &v->weights[0];
				curweight = &curv->weights[0];
				
				// Now copy all the weights
				for(k = 0; k < v->numWeights; k++)
				{
					weight->boneIndex = LittleLong(curweight->boneIndex);
					weight->boneWeight = LittleFloat(curweight->boneWeight);
					
					weight->offset[0] = LittleFloat(curweight->offset[0]);
					weight->offset[1] = LittleFloat(curweight->offset[1]);
					weight->offset[2] = LittleFloat(curweight->offset[2]);
					
					weight++;
					curweight++;
				}
				
				v = (mdrVertex_t *) weight;
				curv = (mdrVertex_t *) curweight;
			}
						
			// we know the offset to the triangles now:
			tri = (mdrTriangle_t *) v;
			surf->ofsTriangles = (int)((byte *) tri - (byte *) surf);
			curtri = (mdrTriangle_t *)((byte *) cursurf + LittleLong(cursurf->ofsTriangles));
			
			// simple bounds check
			if(surf->numTriangles < 0 || (byte *) (tri + surf->numTriangles) > (byte *) mdr + size)
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}

			for(j = 0; j < surf->numTriangles; j++)
			{
				tri->indexes[0] = LittleLong(curtri->indexes[0]);
				tri->indexes[1] = LittleLong(curtri->indexes[1]);
				tri->indexes[2] = LittleLong(curtri->indexes[2]);
				
				tri++;
				curtri++;
			}
			
			// tri now points to the end of the surface.
			surf->ofsEnd = (byte *) tri - (byte *) surf;
			surf = (mdrSurface_t *) tri;

			// find the next surface.
			cursurf = (mdrSurface_t *) ((byte *) cursurf + LittleLong(cursurf->ofsEnd));
		}

		// surf points to the next lod now.
		lod->ofsEnd = (int)((byte *) surf - (byte *) lod);
		lod = (mdrLOD_t *) surf;

		// find the next LOD.
		curlod = (mdrLOD_t *)((byte *) curlod + LittleLong(curlod->ofsEnd));
	}
	
	// lod points to the first tag now, so update the offset too.
	tag = (mdrTag_t *) lod;
	mdr->ofsTags = (int)((byte *) tag - (byte *) mdr);
	curtag = (mdrTag_t *) ((byte *)pinmodel + LittleLong(pinmodel->ofsTags));

	// simple bounds check
	if(mdr->numTags < 0 || (byte *) (tag + mdr->numTags) > (byte *) mdr + size)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
		return qfalse;
	}
	
	for (i = 0 ; i < mdr->numTags ; i++)
	{
		tag->boneIndex = LittleLong(curtag->boneIndex);
		Q_strncpyz(tag->name, curtag->name, sizeof(tag->name));
		
		tag++;
		curtag++;
	}
	
	// And finally we know the real offset to the end.
	mdr->ofsEnd = (int)((byte *) tag - (byte *) mdr);

	// phew! we're done.
	
	return qtrue;
}


static md3Tag_t *R_GetTag(md3Header_t *mod, int frame, const char *tagName)
{
	md3Tag_t *tag;
	int	i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

static void R_GetAnimTag( mdrHeader_t *mod, int framenum, const char *tagName, md3Tag_t * dest) 
{
	int				i, j, k;
	int				frameSize;
	mdrFrame_t		*frame;
	mdrTag_t		*tag;

	if ( framenum >= mod->numFrames ) 
	{
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mod->numFrames - 1;
	}

	tag = (mdrTag_t *)((byte *)mod + mod->ofsTags);
	for ( i = 0 ; i < mod->numTags ; i++, tag++ )
	{
		if ( !strcmp( tag->name, tagName ) )
		{
			Q_strncpyz(dest->name, tag->name, sizeof(dest->name));

			// uncompressed model...
			//
			frameSize = (intptr_t)( &((mdrFrame_t *)0)->bones[ mod->numBones ] );
			frame = (mdrFrame_t *)((byte *)mod + mod->ofsFrames + framenum * frameSize );

			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					dest->axis[j][k]=frame->bones[tag->boneIndex].matrix[k][j];
			}

			dest->origin[0]=frame->bones[tag->boneIndex].matrix[0][3];
			dest->origin[1]=frame->bones[tag->boneIndex].matrix[1][3];
			dest->origin[2]=frame->bones[tag->boneIndex].matrix[2][3];				

			return;
		}
	}
    memset(dest, 0, sizeof(md3Tag_t));
}




//===============================================================================

model_t	*R_GetModelByHandle( qhandle_t index )
{
	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	return tr.models[index];
}

//===============================================================================

model_t *R_AllocModel( void )
{
	model_t	*mod;

	if( tr.numModels == MAX_MOD_KNOWN )
		return NULL;

	mod = ri.Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
====================
RE_RegisterModel: Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an optimization to prevent disk rescanning if they are asked for again.
====================
*/


void R_Modellist_f( void )
{
	int		i, j;
	int		total = 0;

	for ( i = 1 ; i < tr.numModels; i++ )
    {
		model_t	*mod = tr.models[i];
		int lods = 1;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ )
        {
			if ( mod->md3[j] && mod->md3[j] != mod->md3[j-1] )
            {
				lods++;
			}
		}
		ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );
}



//=============================================================================
// leilei - wrapper function to get alternate models loaded

qhandle_t RE_RegisterModel( const char *name )
{
	return RE_RegisterModelReal( name );	// OK!!!
}


int R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagName )
{
	md3Tag_t	*start, *end;
	md3Tag_t	start_space, end_space;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( !model->md3[0] )
	{
		if(model->type == MOD_MDR)
		{
			start = &start_space;
			end = &end_space;
			R_GetAnimTag((mdrHeader_t *) model->modelData, startFrame, tagName, start);
			R_GetAnimTag((mdrHeader_t *) model->modelData, endFrame, tagName, end);
		}
		else if( model->type == MOD_IQM ) {
			return R_IQMLerpTag( tag, model->modelData,
					startFrame, endFrame,
					frac, tagName );
		} else {
            memset(tag, 0, sizeof(orientation_t));
			return qfalse;

		}
	}
	else
	{
		start = R_GetTag( model->md3[0], startFrame, tagName );
		end = R_GetTag( model->md3[0], endFrame, tagName );

		if ( !start || !end ) {
            memset(tag, 0, sizeof(orientation_t));

			return qfalse;
		}
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}


void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs )
{
	model_t* model = R_GetModelByHandle( handle );

	if(model->type == MOD_BRUSH)
	{
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}
	else if (model->type == MOD_MESH)
	{
		md3Header_t* header = model->md3[0];
		md3Frame_t* frame = (md3Frame_t *) ((byte *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		return;
	}
	else if (model->type == MOD_MDR)
	{
		mdrHeader_t* header = (mdrHeader_t *)model->modelData;
		mdrFrame_t* frame = (mdrFrame_t *) ((byte *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		return;
	}
	else if(model->type == MOD_IQM)
	{
		iqmData_t *iqmData = model->modelData;

		if(iqmData->bounds)
		{
			VectorCopy(iqmData->bounds, mins);
			VectorCopy(iqmData->bounds + 3, maxs);
			return;
		}
	}

	VectorClear( mins );
	VectorClear( maxs );
}

void R_ModelInit( void )
{
	model_t	*mod = ri.Hunk_Alloc( sizeof( *tr.models[0] ), h_low );
	// leave a space for NULL model
	mod->index = tr.numModels = 0;
    mod->type = MOD_BAD;

	tr.models[tr.numModels] = mod;
	tr.numModels++;
}
