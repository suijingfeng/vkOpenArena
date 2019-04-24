#include <stdint.h>
#include <stdlib.h>
#include "ref_import.h"
#include "R_Parser.h"


#define MAX_SHADERTEXT_HASH		2048
static char** shaderTextHashTable[MAX_SHADERTEXT_HASH] = { 0 };


static uint32_t GenHashValue( const char *fname, const uint32_t size )
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



/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for the given 
shader name. If found, it will return a valid shader, return NULL if not found.
=====================
*/


char* FindShaderInShaderText( const char * pShaderName, char * const pShaderText )
{

//    ri.Printf( PRINT_ALL, "FindShaderInShaderText: %s\n", pShaderName);

	uint32_t hash = GenHashValue(pShaderName, MAX_SHADERTEXT_HASH);

    uint32_t i;
	for (i = 0; shaderTextHashTable[hash][i]; ++i)
    {
		// str pointed by p is tolower-ed
        char* p = shaderTextHashTable[hash][i];
        // token is the shader name ???
		char* token = R_ParseExt(&p, qtrue);
		// if ( !Q_stricmp( token, pShaderName ) )
        if ( 0 == strcmp( token, pShaderName ) )
        {
			return p;
		}
	}



	if ( pShaderText == NULL)
    {
        // this shouldn't be happen
        ri.Printf( PRINT_ALL, "FindShaderInShaderText: pShaderText=NULL\n");
		return NULL;
	}

    char* p = pShaderText;


	// look for label
	while ( 1 )
    {
		char* token = R_ParseExt( &p, qtrue );
		
        if( token[0] == 0 )
        {
			break;
		}

		if ( !Q_stricmp( token, pShaderName ) )
        {
			return p;
		}
		else
        {
			// skip the definition, tr_common
            // -> R_SkipBracedSection ?
			SkipBracedSection( &p , 0);
		}
	}

	return NULL;
}



void SetShaderTextHashTableSizes( char * const pText )
{
    ri.Printf( PRINT_ALL, "SetShaderTextHashTableSizes \n");

    uint32_t SizeShaderTextHashTable[MAX_SHADERTEXT_HASH] = { 0 };

    uint32_t size = 0;
{    
	// look for shader names
    // text in { } are skiped
    char * p = pText;

	while ( 1 )
    {
		char* token = R_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
        {
			break;
		}

        //ri.Printf( PRINT_ALL, "token: %s\n", token);

        uint32_t hash = GenHashValue(token, MAX_SHADERTEXT_HASH);

        SizeShaderTextHashTable[hash]++;
        size++;
        SkipBracedSection(&p, 0);
    }
}


	size += MAX_SHADERTEXT_HASH;
    

    char* hashMem = (char*)ri.Hunk_Alloc( size * sizeof(char *), h_low );
    
    uint32_t i;
    for (i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        shaderTextHashTable[i] = (char **) hashMem;
        hashMem += (SizeShaderTextHashTable[i] + 1) * sizeof(char *);
    }


	memset(SizeShaderTextHashTable, 0, sizeof(SizeShaderTextHashTable));

{
	char * p = pText;
	// look for shader names
	while ( 1 )
    {
		char* oldp = p;
		char* token = R_ParseExt( &p, qtrue );

		if ( token[0] == 0 ) {
			break;
		}

		uint32_t hash = GenHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][SizeShaderTextHashTable[hash]++] = oldp;

		SkipBracedSection(&p, 0);
	}
}
}
