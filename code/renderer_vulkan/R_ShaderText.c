#include <stdint.h>
#include <stdlib.h>
#include "ref_import.h"
#include "R_Parser.h"
#include "R_SortAlgorithm.h"
#include "R_PrintMat.h"

struct ShaderTextHashArray_s {
    char strName[64]; // char * ppShaderName[count]; but count variary
    char * pNameLoc;
    struct ShaderTextHashArray_s * next;
};

// increase to 4096 
#define MAX_SHADERTEXT_HASH		4096
static struct ShaderTextHashArray_s * s_ShaderNameTab[MAX_SHADERTEXT_HASH];


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

        hash += letter * (i+79);
        ++i;
    }

//  this line doesn't improve the result 
//  hash = (hash ^ (hash >> 10) ^ (hash >> 20));

    return (hash & (size-1));
}


static void printShaderTextHashTableImpl(struct ShaderTextHashArray_s * pTab[MAX_SHADERTEXT_HASH])
{
    uint32_t i = 0;
    uint32_t count = 0;
    uint32_t total = 0;

    int32_t tmpTab[MAX_SHADERTEXT_HASH] = {0};
    
    ri.Printf(PRINT_ALL, "\n\n-----------------------------------------------------\n"); 
    
   
    for(i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        while(pTab[i] != NULL)
        {
            ++tmpTab[i]; 
            ++count;
            pTab[i] = pTab[i]->next;
        }
    }

    quicksort(tmpTab, 0, MAX_SHADERTEXT_HASH-1);


    uint32_t collisionCount = 0;
    for(i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        total += tmpTab[i];
        if(tmpTab[i] > 1)
        {
            // ri.Printf(PRINT_ALL, "%d, ", tmpTab[i]);
            ++collisionCount;
            //if(collisionCount % 16 == 0)
            //    ri.Printf(PRINT_ALL, "\n");
        }
    }

    ri.Printf(PRINT_ALL, "\n Total %d Shaders, hash Table usage: %d/%d, Collision:%d \n",
            total, count, MAX_SHADERTEXT_HASH, collisionCount);
    
    ri.Printf(PRINT_ALL, "\n Top 10 Collision: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
        tmpTab[0], tmpTab[1], tmpTab[2], tmpTab[3], tmpTab[4],
        tmpTab[5], tmpTab[6], tmpTab[7], tmpTab[8], tmpTab[9]);

    ri.Printf(PRINT_ALL, "-----------------------------------------------------\n\n"); 
}


void printShaderTextHashTable_f(void)
{
    printShaderTextHashTableImpl(s_ShaderNameTab);
}




/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for the given 
shader name. If found, it will return a valid shader, return NULL if not found.
=====================
*/
char* FindShaderInShaderText(const char * const pStr)
{
    uint32_t hash = GenHashValue(pStr, MAX_SHADERTEXT_HASH);
    
    const struct ShaderTextHashArray_s * pRoot = s_ShaderNameTab[hash];

    for ( ; pRoot != NULL; pRoot = pRoot->next)
    {
        if ( 0 == Q_stricmp( pRoot->strName, pStr ) )
        {
            return pRoot->pNameLoc;
        }
    }
    return NULL;
}


/*
static void allocateMemoryForHashTable(uint32_t size, struct ShaderTextHashArray_t* const pTable)
{
    uint32_t i;

    char * pMemOffset = (char *) ri.Hunk_Alloc( size * sizeof(char *), h_low );
    memset(pMemOffset, 0, size * sizeof(char *));

    for (i = 0; i < MAX_SHADERTEXT_HASH; ++i)
    {
        // There no need to allocate memory for the NOT use hash table
        if(pTable[i].Count != 0)
        {
            pTable[i].ppShaderName = (char * *)pMemOffset;
            pMemOffset += pTable[i].Count * sizeof(char *);
            // ri.Hunk_Alloc( pTable[i].Count * sizeof(char *), h_low );
        }
    }
}
*/


// Doing things this way cause pText got parsed twice,
// can we parse only once?
static void FillHashTableWithShaderNames(char * pText, struct ShaderTextHashArray_s ** const pTable)
{
    //char * p = pText;

    while ( 1 )
    {
        // char* oldp = p;

        // look for shader names
        char* token = R_ParseExt( & pText, qtrue );

        if ( token[0] == 0 ) {
            break;
        }
        
        struct ShaderTextHashArray_s* pRoot = (struct ShaderTextHashArray_s* )
            ri.Hunk_Alloc( sizeof(struct ShaderTextHashArray_s), h_low );

        strcpy(pRoot->strName, token);
        uint32_t hash = GenHashValue(pRoot->strName, MAX_SHADERTEXT_HASH);
        pRoot->pNameLoc = pText;
        pRoot->next = pTable[hash];

        pTable[hash] = pRoot;

        // "{ ... }" are skiped.
        SkipBracedSection(&pText, 0);
    }
}

void SetShaderTextHashTableSizes(char * const pText)
{
    ri.Printf( PRINT_ALL, "SetShaderTextHashTableSizes. \n");

    memset( s_ShaderNameTab, 0, sizeof(s_ShaderNameTab) );
    
    // look for shader names
    FillHashTableWithShaderNames(pText, s_ShaderNameTab);
}
