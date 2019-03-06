/*
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "../matrix_multiplication.h"
typedef float mat4_t[16];
void Mat4Copy1( const mat4_t in, mat4_t out )
{
    memcpy(out, in, 64); 
}

void Mat4Copy2( const mat4_t in, mat4_t out )
{
	out[ 0] = in[ 0]; out[ 4] = in[ 4]; out[ 8] = in[ 8]; out[12] = in[12]; 
	out[ 1] = in[ 1]; out[ 5] = in[ 5]; out[ 9] = in[ 9]; out[13] = in[13]; 
	out[ 2] = in[ 2]; out[ 6] = in[ 6]; out[10] = in[10]; out[14] = in[14]; 
	out[ 3] = in[ 3]; out[ 7] = in[ 7]; out[11] = in[11]; out[15] = in[15]; 
}


void MM4x4Print(float M[16])
{
    
    printf("\n");
    int i = 0;
    for(i = 0; i<16; i++)
    {
        if(i%4 == 0)
            printf("\n");

        printf(" %f,\t", M[i]);
    }
    printf("\n");
}



int main(int argc, char *argv[])
{
    struct timeval tv_begin, tv_end;

    int n = 0, netTimeMS = 0;
    
    int runCount = 10000000;

    float A[16] = {
    1.1, 2.2, 3.3, 4.4, 5.5, 6.0, 7.1, 8.3,
    0.1, 0.2, 0.3, 0.4, 2.5, 3.0, 4.1, 5.3,    
    };

    float B[16] = {
    9.1, 3.2, 5.3, 4.3, 5.4, 6.9, 4.1, 1.3,
    0.0, 1.2, 2.3, 7.4, 6.4, 7.0, 6.1, 2.3,    
    };

    float C[16] = {0};
    float out[16] = {0};



    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            Mat4Copy1(A, C);
            A[n%16] = (n%16)/10.0;
        }
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;

        MM4x4Print(C);
        printf("MatrixMultiply4x4: %d\n", netTimeMS);

    }


    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {    
            Mat4Copy2(B, out);
            B[n%16] = (n%16)/10.0;
        }
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;

        MM4x4Print(out);
        printf("MatrixMultiply4x4_SSE: %d\n", netTimeMS);

    }

    return 1;
}


