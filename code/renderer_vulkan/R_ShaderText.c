#include <stdint.h>
#include <stdlib.h>
#include "ref_import.h"
#include "R_Parser.h"
#include "R_SortAlgorithm.h"


struct ShaderTextHashArray_t {
    char ** ppShaderName; // char * ppShaderName[count]; but count variary
    uint32_t Count;
};

// increase to 4096 ?
#define MAX_SHADERTEXT_HASH		4096

static struct ShaderTextHashArray_t s_ShaderNameTab[MAX_SHADERTEXT_HASH] = {0};


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

        hash += letter * (i+89);
        ++i;
    }

//    hash = (hash ^ (hash >> 10) ^ (hash >> 20));
    return (hash & (size-1));
}



void printShaderTextHashTable_f(void)
{
    uint32_t i = 0;
    uint32_t count = 0;
    uint32_t total = 0;

    uint32_t tmpTab[MAX_SHADERTEXT_HASH] = {0};
    ri.Printf(PRINT_ALL, "\n\n-----------------------------------------------------\n"); 
    
   
    for(i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        tmpTab[i] = s_ShaderNameTab[i].Count;
        if(tmpTab[i]) {
            ++count;
            total += tmpTab[i];
        }
    }

    quicksort(tmpTab, 0, MAX_SHADERTEXT_HASH-1);


    uint32_t collisionCount = 0;
    for(i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        if(tmpTab[i] > 1)
        {
            ri.Printf(PRINT_ALL, "%d, ", tmpTab[i]);
            ++collisionCount;
            if(collisionCount % 16 == 0)
                ri.Printf(PRINT_ALL, "\n");
        }
    }

    ri.Printf(PRINT_ALL, "\n Total %d Shaders, hash Table usage: %d/%d, Collision:%d \n",
            total, count, MAX_SHADERTEXT_HASH, collisionCount);
    

    ri.Printf(PRINT_ALL, "-----------------------------------------------------\n\n"); 
}


/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for the given 
shader name. If found, it will return a valid shader, return NULL if not found.
=====================
*/


char* FindShaderInShaderText(const char * pStr)
{
    uint32_t hash = GenHashValue(pStr, MAX_SHADERTEXT_HASH);

    uint32_t i;


    uint32_t Count = s_ShaderNameTab[hash].Count;
    char ** const ppNames = s_ShaderNameTab[hash].ppShaderName;
    for (i = 0; i < Count; ++i)
    {
        // str pointed by p is tolower-ed
        const char* p = ppNames[i];
        // token is the shader name ???
        char* token = R_ParseExt(&p, qtrue);
        // if ( !Q_stricmp( token, pShaderName ) )
        if ( 0 == Q_stricmp( token, pStr ) )
        {
            return p;
        }
    }

    return NULL;
}


// return total number of the shader name, token size in short
// fill the count to every item in SizeTable
// pText point to the shader text in start
static uint32_t HashCollisionCount(const char * pText, struct ShaderTextHashArray_t* pTable)
{    
    // look for shader names
    uint32_t tSize = 0;
    while ( 1 )
    {
        char* token = R_ParseExt( &pText, qtrue );
        if ( token[0] == 0 )
        {
            break;
        }

        ++tSize;

        uint32_t hash = GenHashValue(token, MAX_SHADERTEXT_HASH);

        ++pTable[hash].Count;

        // "{ ... }" are skiped.
        SkipBracedSection(&pText, 0);
    }
    return tSize;
}


static void allocateMemoryForHashTable(struct ShaderTextHashArray_t* const pTable)
{
    uint32_t i;
    for (i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        // "+1": the last element in shaderTextHashTable[i] save NULL
        // indicate that end of Collision.
        //
        // TODO:
        // There no need to allocate memory for the NOT use hash table
        // We refuse allocate memory for it ???
        if(pTable[i].Count != 0)
            pTable[i].ppShaderName = (char **) 
                ri.Hunk_Alloc( pTable[i].Count * sizeof(char *), h_low );
    }
}


static void FillHashTableWithShaderNames(const char * pText, struct ShaderTextHashArray_t* const pTable)
//        char ** ppTable[MAX_SHADERTEXT_HASH])
{

    uint32_t itemsCountTab[MAX_SHADERTEXT_HASH] = { 0 };
    const char * p = pText;

    while ( 1 )
    {
        const char* oldp = p;

        // look for shader names
        const char* token = R_ParseExt( &p, qtrue );

        if ( token[0] == 0 ) {
            break;
        }

        uint32_t hash = GenHashValue(token, MAX_SHADERTEXT_HASH);

        //ppTable[hash][itemsCountTab[hash]++] = oldp;
        pTable[hash].ppShaderName[itemsCountTab[hash]++] = oldp;

        // "{ ... }" are skiped.
        SkipBracedSection(&p, 0);
    }
}


void SetShaderTextHashTableSizes(const char * const pText)
{
    ri.Printf( PRINT_ALL, "SetShaderTextHashTableSizes \n");

    memset(s_ShaderNameTab, 0, sizeof(s_ShaderNameTab));
    // look for shader names
    // text in { } are skiped
    uint32_t size = HashCollisionCount(pText, s_ShaderNameTab);
    ri.Printf( PRINT_ALL, "shader name token size: %d\n", size);

    allocateMemoryForHashTable(s_ShaderNameTab);
    FillHashTableWithShaderNames(pText, s_ShaderNameTab);
}
