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
#include<sys/time.h>
#include "../matrix_multiplication.h"
void myGlMultMatrix(const float A[16], const float B[16], float out[16]);
void MatrixMultiply4x4(const float A[16], const float B[16], float out[16]);
void MatrixMultiply4x4_ASM(const float A[16], const float B[16], float out[16]);

/*
void matMul( float result[4][4], float left[4][4], float right[4][4] )
{
    __asm
    {
        mov eax,left
        mov edi,right
        mov edx,result
        movaps xmm4,[edi]
        movaps xmm5,[edi+16]
        movaps xmm6,[edi+32]
        movaps xmm7,[edi+48]
        mov edi,0
        mov ecx,4
l_:     movaps xmm0,[eax+edi]
        movaps xmm1,xmm0
        movaps xmm2,xmm0
        movaps xmm3,xmm0
        shufps xmm0,xmm0,00000000b
        shufps xmm1,xmm1,01010101b
        shufps xmm2,xmm2,10101010b
        shufps xmm3,xmm3,11111111b
        mulps xmm0,xmm4
        mulps xmm1,xmm5
        mulps xmm2,xmm6
        mulps xmm3,xmm7
        addps xmm2,xmm0
        addps xmm3,xmm1
        addps xmm3,xmm2
        movaps [edx+edi],xmm3
        add edi,16
        loop l_
    }
}
*/



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
    float out2[16] = {0};
    float out3[16] = {0};

    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            MatrixMultiply4x4(A, B, C);
            A[n%16] = (n%16)/3+0.00001;
            B[n%16] = (n%16)/2+0.00002;

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
            MatrixMultiply4x4_SSE(A, B, out);
            A[n%16] = (n%16)/3+0.00001;
            B[n%16] = (n%16)/2+0.00002;
        }
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;

        MM4x4Print(out);
        printf("MatrixMultiply4x4_SSE: %d\n", netTimeMS);

    }


    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            myGlMultMatrix(A, B, out2);
            A[n%16] = (n%16)/3+0.00001;
            B[n%16] = (n%16)/2+0.00002;
        }
        
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
        MM4x4Print(out2);
        printf("myGlMultMatrix: %d\n", netTimeMS);

    }



    {

        gettimeofday(&tv_begin, NULL);

        for(n = 0; n < runCount; n++ )
        {
            MatrixMultiply4x4_ASM(B, A, out3);
            A[n%16] = (n%16)/3+0.00001;
            B[n%16] = (n%16)/2+0.00002;
        }
        gettimeofday(&tv_end, NULL);

        netTimeMS = (tv_end.tv_sec - tv_begin.tv_sec) * 1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
        MM4x4Print(out3);
        printf("MatrixMultiply4x4_ASM: %d\n", netTimeMS);
    }


    return 1;
}


