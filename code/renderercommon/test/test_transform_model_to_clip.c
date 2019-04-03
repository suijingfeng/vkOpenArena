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


void TransformModelToClip_SSE2( const float src[3], const float pMatModel[16], const float pMatProj[16], float dst[4] );



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
    
    int runCount = 10000000;

    //float x[4] QALIGN(16) = {1.0f, -1.0f, 2.0f, 3.0f}; 

    float out1[4] QALIGN(16) = {0};
    float out2[4] QALIGN(16) = {0};

    float src[3] QALIGN(16) = {1336.000000, -968.000000, 304.000000};
    float MatModel[16] QALIGN(16) = {-0.074783, 0.229112, -0.970523, 0.000000, -0.997178, -0.010732, 0.074304, 0.000000, 0.006609, 0.973341, 0.229268, 0.000000, -842.976501, -487.258972, 1035.254395, 1.000000};
    float MatProj[16] QALIGN(16) = {0.363970, 0.000000, 0.000000, 0.000000, 0.000000, 0.647058, 0.000000, 0.000000, 0.000000, 0.000000, -1.012096, -1.000000, 0.000000, 0.000000, -8.048385, 0.000000};

    float dst[4] QALIGN(16) = {8.877327, 80.959183, 258.733032, 263.592773};


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
        float notuse[16];
        for(n = 0; n < runCount; n++ )
        {
            MatModel[n%16] = (n%16)/3.0f-0.000001;
            
            MatProj[n%16] = (n%16)/2+0.000001;

            src[n%4] = n%4 + 0.0000001;
            TransformModelToClip(src, MatModel, MatProj, notuse, out2);
        }
        
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
        //MM4x1Print("out2", out2);
        printf("TransformModelToClip: %d\n", netTimeMS);
    }


    {
        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            MatModel[n%16] = (n%16)/3.0f-0.000001;
            
            MatProj[n%16] = (n%16)/2+0.000001;
            src[n%4] = n%4 + 0.0000001;

            TransformModelToClip_SSE(src, MatModel, MatProj, out1);
        }
        
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
        //MM4x1Print("out1", out1);
        printf("TransformModelToClip_SSE: %d\n", netTimeMS);
    }


    {
        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            MatModel[n%16] = (n%16)/3.0f-0.000001;
            
            MatProj[n%16] = (n%16)/2+0.000001;
            src[n%4] = n%4 + 0.0000001;

  
            TransformModelToClip_SSE2(src, MatModel, MatProj, dst);
        }
        
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
        //MM4x1Print("dst", dst);
        printf("TransformModelToClip_SSE2: %d\n", netTimeMS);
    }


    return 1;
}


