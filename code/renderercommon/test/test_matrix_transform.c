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

#define QALIGN(x) __attribute__((aligned(x)))


void myGlMultMatrix( const float *a, const float *b, float *out );



void MM4x1Print(float M[4])
{
    
    printf("\n");
    int i = 0;
    for(i = 0; i<4; i++)
    {
        printf(" %f,\t", M[i]);
    }
    printf("\n");
}



int main(int argc, char *argv[])
{
    struct timeval tv_begin, tv_end;

    int n = 0, netTimeMS = 0;
    
    int runCount = 100000000;

    float A[16] QALIGN(16)= {
    1.1, 2.2, 3.3, 4.4, 5.5, 6.0, 7.1, 8.3,
    0.1, 0.2, 0.3, 0.4, 2.5, 3.0, 4.1, 5.3,    
    };

    float B[16] QALIGN(16)= {
    9.1, 3.2, 5.3, 4.3, 5.4, 6.9, 4.1, 1.3,
    0.0, 1.2, 2.3, 7.4, 6.4, 7.0, 6.1, 2.3,    
    };

    float x[4] QALIGN(16) = {1.0f, -1.0f, 2.0f, 3.0f}; 

    float out1[4] QALIGN(16) = {0};
    float out2[4] QALIGN(16) = {0};

/*
    Mat4Transform(A, x, out1);
    MM4x1Print(out1);
    Mat4x1Transform_SSE(A, x, out2);
    MM4x1Print(out2);

    
    Mat4Transform(B, x, out1);
    MM4x1Print(out1);
    Mat4x1Transform_SSE(B, x, out2);
    MM4x1Print(out2);
*/


    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            Mat4Transform(B, x, out2);
            B[n%16] = (n%16)/3.0f;
            x[n%4] = (n%4)/3.0f;  
        }
        
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
        MM4x1Print(out2);
        printf("Mat4Transform: %d\n", netTimeMS);

    }


    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {    
            Mat4x1Transform_SSE(A, x, out1);
            A[n%16] = (n%16)/3.0f;
            x[n%4] = (n%4)/3.0f;  
        }
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;

        MM4x1Print(out1);
        printf("Mat4x1Transform_SSE: %d\n", netTimeMS);

    }

    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {    
            Vec4Transform_SSE(A, x, out1);
            A[n%16] = (n%16)/3.0f;
            x[n%4] = (n%4)/3.0f;  
        }
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;

        MM4x1Print(out1);
        printf("Vec4Transform_SSE: %d\n", netTimeMS);

    }

    return 1;
}


