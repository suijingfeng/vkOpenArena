#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* getExtension( const char *name );

char* getExtension( const char *name )
{
	char* dot = strrchr(name, '.');
    char* slash = strrchr(name, '/');

	if ((dot != NULL) && ((slash == NULL) || (slash < dot) ))
		return dot + 1;
	else
		return "";
}


int main(int argc, char *argv[])
{
	char *name = "models/powerups/trailtest.md3";
	
	char *ext = getExtension(name);
	
	if( *ext )
		printf("extension: %s\n", ext);
	else
		printf("error pass test!\n");

	return 0;
}

