#include "vk_image.h"
#include "tr_cvar.h"
#include "ref_import.h"

#include "R_PrintMat.h"
#include "tr_globals.h"
#include "R_Parser.h"
#include "R_ShaderText.h"

#define FILE_HASH_SIZE		1024
static shader_t* hashTable[FILE_HASH_SIZE] = {0};


static char * s_pShaderText = NULL;

/*
================
return a hash value for the filename
================
*/
static uint32_t generateHashValue( const char *fname, const uint32_t size )
{
	uint32_t i = 0;
	uint64_t hash = 0;

	while (fname[i] != '\0')
    {
		uint32_t letter = tolower(fname[i]);
		
        if (letter == '.')
            break;				// don't include extension
		if ((letter =='\\') || (letter == PATH_SEP))
            letter = '/';		// damn path names

        hash += letter * (i+119);
		++i;
	}
    
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return (hash & (size-1));
}



void R_ClearShaderHashTable()
{
	memset(hashTable, 0, sizeof(hashTable));
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/


shader_t* R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage )
{


	if ( name == NULL )
    {
        ri.Printf( PRINT_WARNING, "Find Shader: name = NULL\n");
		return tr.defaultShader;
	}


	// use (fullbright) vertex lighting if the bsp file doesn't have lightmaps
	if ( (lightmapIndex >= 0) && (lightmapIndex >= tr.numLightmaps) )
    {
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}
    else if ( lightmapIndex < LIGHTMAP_2D )
    {
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri.Printf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndex  );
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}
   

    char strippedName[MAX_QPATH] = {0};

	R_StripExtension(name, strippedName, sizeof(strippedName));


    uint32_t hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
    //
    {
        shader_t* sh = hashTable[hash];
        while ( sh )
        {
            // NOTE: if there was no shader or image available with the name strippedName
            // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
            // have to check all default shaders otherwise for every call to R_findShader
            // with that same strippedName a new default shader is created.
            if ( ( 0 == Q_stricmp(sh->name, strippedName) ) && ((sh->lightmapIndex == lightmapIndex) || sh->defaultShader) )
            {
                // match found
                return sh;
            }

            sh = sh->next;
        }
    }
  
    R_SetTheShader( strippedName, lightmapIndex );

	//
	// attempt to define shader from an explicit parameter file
	//
    //char* pShaText = FindShaderInShaderText( strippedName, s_pShaderText );
    char* pShaText = FindShaderInShaderText( strippedName );
    if ( pShaText )
    {
        // enable this when building a pak file to get a global list
        // of all explicit shaders
        if ( r_printShaders->integer ) {
            ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
        }

        if ( !ParseShader( &pShaText ) )
        {
            // had errors, so use default shader
            R_SetDefaultShader( );
            ri.Printf( PRINT_WARNING, "ParseShader: %s had errors\n", strippedName );
        }

        return FinishShader();
    }


	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file

    image_t* image = NULL;
    
    char fileName[MAX_QPATH] = {0};
    strncpy(fileName, name, MAX_QPATH);


    if( (fileName[1] == ':') && (
        (fileName[0] = 'C') || 
        (fileName[0] = 'D') ||
        (fileName[0] = 'E') || 
        (fileName[0] = 'F') ) )
        ri.Printf( PRINT_WARNING, "Ugly shader name: %s\n", fileName );
    else if( (fileName[0] == '/') && 
        (fileName[1] = 'h') && (fileName[2] = 'o') &&
        (fileName[3] = 'm') && (fileName[4] = 'e') )
    {
        ri.Printf( PRINT_WARNING, "Ugly shader name: %s\n", fileName );
    }
    else {
        image = R_FindImageFile( name, mipRawImage, mipRawImage,
                mipRawImage ? GL_REPEAT : GL_CLAMP );
    }


    if(image != NULL)
    {
        // create the default shading commands
        R_CreateDefaultShadingCmds(name, image);
    }
    else
	{
	    R_SetDefaultShader();
	}


    return FinishShader();
}






/* 
====================
This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name )
{

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf(PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

    shader_t* sh = R_FindShader( name, LIGHTMAP_2D, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}

/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name )
{

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf(PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}
 
	shader_t* sh = R_FindShader( name, LIGHTMAP_2D, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage)
{

	int hash = generateHashValue(name, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
    shader_t* sh = hashTable[hash];
	while(sh)
    {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( (sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
			// index by name
			!Q_stricmp(sh->name, name)) {
			// match found
			return sh->index;
		}

        sh = sh->next;
	}

    R_SetTheShader( name, lightmapIndex );

	//
	// create the default shading commands
	//
    R_CreateDefaultShadingCmds(name, image);

	sh = FinishShader();
    return sh->index; 
}



static void BuildSingleLargeBuffer(char* buffers[], const int nShaderFiles, const int sum)
{
    ri.Printf( PRINT_ALL, " Concatenate %d shader files into a single large buffer. \n", nShaderFiles);
    
    // nShaderFiles*2
    // one for "\n", another for what ?
    s_pShaderText = ri.Hunk_Alloc( sum + nShaderFiles*2, h_low );

	memset( s_pShaderText, 0, sum + nShaderFiles*2 );

    char* pTextEnd = s_pShaderText;
    
    int n;
    // free in reverse order, so the temp files are all dumped
    for ( n = nShaderFiles - 1; n >= 0 ; --n )
    {
        if ( buffers[n] )
        {
            strcat( pTextEnd, buffers[n] );
            strcat( pTextEnd, "\n" );

            pTextEnd += strlen(buffers[n]) + 1;

            ri.FS_FreeFile( buffers[n] );
        }
    }
}


static void Shader_DoSimpleCheck(char* name, char* p)
{
    char* pBuf = p;

    R_BeginParseSession(name);

    while(1)
    {
        char* token = R_ParseExt(&p, qtrue);
        if(0 == *token)
            break;
        
        char shaderName[MAX_QPATH]={0};
        strncpy(shaderName, token, sizeof(shaderName));

        int shaderLine = R_GetCurrentParseLine();

        token = R_ParseExt(&p, qtrue);
        if((token[0] != '{') || (token[1] != '\0'))
        {
            ri.Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing opening brace",
                    name, shaderName, shaderLine);
            if (token[0])
            {
                ri.Printf(PRINT_WARNING, " (found \"%s\" on line %d)", token, R_GetCurrentParseLine());
            }
            ri.Printf(PRINT_WARNING, ".\n");
            ri.FS_FreeFile(pBuf);
            pBuf = NULL;
            break;
        }

        if(!SkipBracedSection(&p, 1))
        {
            ri.Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing closing brace.\n",
                    name, shaderName, shaderLine);
            ri.FS_FreeFile(pBuf);
            pBuf = NULL;
            break;
        }
    }
}



/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/

#define	MAX_SHADER_FILES	4096
void ScanAndLoadShaderFiles( void )
{

	uint32_t numShaderFiles = 0;

	// scan for shader files
	char** ppShaderFiles = ri.FS_ListFiles( "scripts", ".shader", &numShaderFiles );

	if ( (ppShaderFiles == NULL) || (numShaderFiles == 0 ))
	{
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES )
    {
		numShaderFiles = MAX_SHADER_FILES;
        ri.Printf( PRINT_WARNING, "numShaderFiles > MAX_SHADER_FILES\n" );
	}


	// load and parse shader files
    ri.Printf( PRINT_ALL, " Scan And Load %d Shader Files \n", numShaderFiles);
   
    long sum = 0;
    char * pBuffers[MAX_SHADER_FILES] = {0};

    uint32_t i;
	for ( i = 0; i < numShaderFiles; ++i )
	{
		char filename[MAX_QPATH] = {0};

		snprintf( filename, sizeof( filename ), "scripts/%s", ppShaderFiles[i] );
		
        // ri.Printf( PRINT_ALL, "...loading '%s'\n", filename );
		
        // summand is the file length
        long summand = ri.FS_ReadFile( filename, &pBuffers[i] );
		
		if ( NULL == pBuffers[i] )
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		
		// Do a simple check on the shader structure in that file
        // to make sure one bad shader file cannot fuck up all other shaders.
	    
        
        Shader_DoSimpleCheck(filename, pBuffers[i]);
		// Do a simple check on the shader structure in that file 
        // to make sure one bad shader file cannot fuck up all other shaders.

		if (pBuffers[i])
			sum += summand;		
	}

	// build single large buffer
    BuildSingleLargeBuffer(pBuffers, numShaderFiles, sum);
   
    FunLogging("BuildSingleLargeBuffer.txt", s_pShaderText);
	
    R_Compress( s_pShaderText );

    FunLogging("after_R_Compress.txt", s_pShaderText);


	// free up memory
	ri.FS_FreeFileList( ppShaderFiles );


    SetShaderTextHashTableSizes( s_pShaderText );

	return;
}


/* 
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/

void RE_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset)
{

    shader_t* sh2 = tr.defaultShader;

    //R_FindShaderByName( newShaderName );
    {
        char strippedName2[MAX_QPATH];
	    R_StripExtension( newShaderName, strippedName2, sizeof(strippedName2) );

	    int hash2 = generateHashValue(strippedName2, FILE_HASH_SIZE);

        // see if the shader is already loaded
        shader_t* pSh = hashTable[hash2];

        while ( pSh )
        {
            // NOTE: if there was no shader or image available with the name strippedName
            // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
            // have to check all default shaders otherwise for every call to R_findShader
            // with that same strippedName a new default shader is created.
            if (Q_stricmp(pSh->name, strippedName2) == 0)
            {
                // match found
                sh2 = pSh;
                break;
            }
            pSh=pSh->next;
        }

        if (sh2 == tr.defaultShader)
        {
            qhandle_t h;
            //h = RE_RegisterShaderLightMap(newShaderName, 0);

            pSh = R_FindShader( newShaderName, 0, qtrue );

            if ( pSh->defaultShader )
            {
                h = 0;
            }
            else
            {
                h = pSh->index;
            }

            sh2 = R_GetShaderByHandle(h);

            if( (sh2 == tr.defaultShader) || (sh2 == NULL) )
            {
                ri.Printf( PRINT_WARNING, "WARNING: shader %s not found\n", newShaderName );
            }
        }
    }
    
    char strippedName[MAX_QPATH];
	R_StripExtension(shaderName, strippedName, sizeof(strippedName));
	int hash = generateHashValue(strippedName, FILE_HASH_SIZE);
    shader_t* sh = hashTable[hash];
	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	
    while( sh )
    {
		if (Q_stricmp(sh->name, strippedName) == 0)
        {
			if (sh != sh2)
            {
				sh->remappedShader = sh2;
			}
            else
            {
				sh->remappedShader = NULL;
			}
		}
        sh = sh->next;
	}

	if (timeOffset)
    {
		sh2->timeOffset = atof(timeOffset);
	}
}




void R_UpdateShaderHashTable(shader_t* pNewShader)
{
	uint32_t hash = generateHashValue(pNewShader->name, FILE_HASH_SIZE);
	pNewShader->next = hashTable[hash];
	hashTable[hash] = pNewShader;
}
