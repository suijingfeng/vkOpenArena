#include <stdio.h>
#include <stdlib.h>

static FILE* log_fp = NULL;


void FunLogging(const char * name, char * pBuf )
{

    log_fp = fopen( name, "wt" );


	if ( log_fp )
	{
		fprintf( log_fp, "%s", pBuf );
	}
    else
    {
        fprintf(stderr, "Error open %s\n", name);
    }

    fclose( log_fp );
	log_fp = NULL;
}
