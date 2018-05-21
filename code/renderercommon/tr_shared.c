/*
===========================================================================
Copyright (C) 2010 James Canete (use.less01@gmail.com)

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
// tr_subs.c - common function replacements for modular renderer

#include "tr_shared.h"
#include <stdio.h>

const float colorBlack[4]	= {0, 0, 0, 1};
const float	colorRed[4]	    = {1, 0, 0, 1};
const float	colorGreen[4]	= {0, 1, 0, 1};
const float	colorBlue[4]	= {0, 0, 1, 1};
const float	colorYellow[4]	= {1, 1, 0, 1};
const float	colorMagenta[4] = {1, 0, 1, 1};
const float	colorCyan[4]	= {0, 1, 1, 1};
const float	colorWhite[4]	= {1, 1, 1, 1};
const float	colorLtGrey[4]	= {0.75, 0.75, 0.75, 1};
const float	colorMdGrey[4]	= {0.5, 0.5, 0.5, 1};
const float	colorDkGrey[4]	= {0.25, 0.25, 0.25, 1};


void AxisClear( float axis[3][3] )
{
    memset(axis, 0, 9 * sizeof(float));
}


char *SkipPath(char *pathname)
{
	char *last = pathname;
    char c;
    do{
        c = *pathname;
    	if (c == '/')
		    last = pathname+1;
        pathname++;
    }while(c);

	return last;
}


void stripExtension(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.');
    const char *slash = strrchr(in, '/');


	if ((dot != NULL) && ( (slash < dot) || (slash == NULL) ) )
    {
        int len = dot-in+1;
        if(len <= destsize)
            destsize = len;
        else
		    fprintf( stderr, "stripExtension: dest size not enough!\n");
    }

    if(in != out)
    	strncpy(out, in, destsize-1);
	
    out[destsize-1] = '\0';
}


const char *getExtension( const char *name )
{
	const char* dot = strrchr(name, '.');
    const char* slash = strrchr(name, '/');

	if ((dot != NULL) && ((slash == NULL) || (slash < dot) ))
		return dot + 1;
	else
		return "";
}
