### void glTranslated(GLdouble x,  GLdouble y,  GLdouble z); 
### void glTranslatef(GLfloat x,  GLfloat y,  GLfloat z);

multiply the current matrix by a translation matrix 

glTranslate produces a translation by `(x, y, z)`
            
The current matrix (see glMatrixMode) is multiplied by this translation matrix,
with the product replacing the current matrix, as if glMultMatrix were called
with the following matrix for its argument:

```
M =
    1 0 0 x
    0 1 0 y
    0 0 1 z
    0 0 0 1
```

If the matrix mode is either GL\_MODELVIEW or GL\_PROJECTION,
all objects drawn after a call to glTranslate are translated.

Parameters: x, y, z, Specify the x, y, and z coordinates of a translation vector.

If you want to specify explicitly a particular matrix to be 
loaded as the current matrix, use `glLoadMatrix*()`. Similarly,
use `glMultMatrix*()` to multiply the current matrix by the 
matrix passed in as an argument. The argument for both these
commands is a vector of sixteen values (m1, m2, ... , m16)
that specifies a matrix M as follows:

M = [ m0 m4 m8  m12 
      m1 m5 m9  m13
      m2 m6 m10 m14
      m3 m7 m11 m15 ]

OpenGL implementations often must compute the inverse of the
modelview matrix so that normals and clipping planes can be
correctly transformed to eye coordinates.

Caution: If you're programming in C and you declare a matrix
as `m[4][4]`, then the element `m[i][j]` is in the ith column
and jth row of the OpenGL transformation matrix. This is the
reverse of the standard C convention in which `m[i][j]` is in
row i and column j. To avoid confusion, you should declare 
your matrices as m[16].
Use glPushMatrix and glPopMatrix to save and restore the untranslated coordinate system.



### void glMultMatrix{fd}(const TYPE *m);

Multiplies the matrix specified by the sixteen values pointed
to by m by the current matrix and stores the result as the current matrix.


Note: All matrix multiplication with OpenGL occurs as follows:
Suppose the current matrix is C and the matrix specified with
`glMultMatrix*()` or any of the transformation commands is M. 
After multiplication, the final matrix is always `C * M`. 

```
Res: result matrix.
Res = C * M;
Res^T =  M^T * C^T;
```


```
qglTranslatef (backEnd.viewParms.or.origin[0], backEnd.viewParms.or.origin[1], backEnd.viewParms.or.origin[2]);
```

is equivalent to the following code according to tr\_sky.c


```
float skybox_translate[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    backEnd.viewParms.or.origin[0], backEnd.viewParms.or.origin[1], backEnd.viewParms.or.origin[2], 1};

float modelview_transform[16];
myGlMultMatrix(skybox_translate, backEnd.viewParms.world.modelMatrix, modelview_transform);
qglLoadMatrixf(modelview_transform);
```


* presume if we want do the product out = A x B mathmatically,
* and the matrix is row-major layout in RAM in C .

```
    matrix A[16] =[ a00 a01 a02 a03
                    a10 a11 a12 a13
                    a20 a21 a22 a23
                    a30 a31 a32 a33 ]

    matrix B[16] ={ b00 b01 b02 b03
                    b10 b11 b12 b13
                    b20 b21 b22 b23
                    b30 b31 b32 b33 }
```

If we pass it to a function by a point and one-dimension array,

A will be layout this way: 

` A[16] = { a00 a01 a02 a03  a10 a11 a12 a13  a20 a21 a22 a23  a30 a31 a32 a33 } `

B will be layout this way: 

` B[16] = { b00 b01 b02 b03  b10 b11 b12 b13  b20 b21 b22 b23  b30 b31 b32 b33 } `

If we want do the product `out = A x B` mathmatically, then out[16] will be: 

```
out[16] = {
    a00 x b00 + a01 * b10 + a02 * b20 + a03 * b30, 
    a00 x b01 + a01 * b11 + a02 * b21 + a03 * b31, 
    a00 x b02 + a01 * b12 + a02 * b22 + a03 * b32, 
    a00 x b03 + a01 * b13 + a02 * b23 + a03 * b33,

    a10 x b00 + a11 * b10 + a12 * b20 + a13 * b30, 
    a10 x b01 + a11 * b11 + a12 * b21 + a13 * b31, 
    a10 x b02 + a11 * b12 + a12 * b22 + a13 * b32, 
    a10 x b03 + a11 * b13 + a12 * b23 + a13 * b33, 

    a20 x b00 + a21 * b10 + a22 * b20 + a23 * b30, 
    a20 x b01 + a21 * b11 + a22 * b21 + a23 * b31, 
    a20 x b02 + a21 * b12 + a22 * b22 + a23 * b32, 
    a20 x b03 + a21 * b13 + a22 * b23 + a23 * b33,

    a30 x b00 + a31 * b10 + a32 * b20 + a33 * b30, 
    a30 x b01 + a31 * b11 + a32 * b21 + a33 * b31, 
    a30 x b02 + a31 * b12 + a32 * b22 + a33 * b32, 
    a30 x b03 + a31 * b13 + a32 * b23 + a33 * b33
    };
    
= { 
    A[0]*B[0] + A[1]*B[4] + A[2]*B[8] + A[3]*B[12],
    A[0]*B[1] + A[1]*B[5] + A[2]*B[9] + A[3]*B[13],
    A[0]*B[2] + A[1]*B[6] + A[2]*B[10] + A[3]*B[14],
    A[0]*B[3] + A[1]*B[7] + A[2]*B[11] + A[3]*B[15],

    A[4]*B[0] + A[5]*B[4] + A[6]*B[8] + A[7]*B[12],
    A[4]*B[1] + A[5]*B[5] + A[6]*B[9] + A[7]*B[13],
    A[4]*B[2] + A[5]*B[6] + A[6]*B[10] + A[7]*B[14],
    A[4]*B[3] + A[5]*B[7] + A[6]*B[11] + A[7]*B[15],

    A[8]*B[0] + A[9]*B[4] + A[10]*B[8] + A[11]*B[12],
    A[8]*B[1] + A[9]*B[5] + A[10]*B[9] + A[11]*B[13],
    A[8]*B[2] + A[9]*B[6] + A[10]*B[10] + A[11]*B[14],
    A[8]*B[3] + A[9]*B[7] + A[10]*B[11] + A[11]*B[15],

    A[12]*B[0] + A[13]*B[4] + A[14]*B[8] + A[15]*B[12],
    A[12]*B[1] + A[13]*B[5] + A[14]*B[9] + A[15]*B[13],
    A[12]*B[2] + A[13]*B[6] + A[14]*B[10] + A[15]*B[14],
    A[12]*B[3] + A[13]*B[7] + A[14]*B[11] + A[15]*B[15]
  };
  
```
   if 
       A[16] =[ a00 a01 a02 a03
                a10 a11 a12 a13
                a20 a21 a22 a23
                a30 a31 a32 a33 ]
   then
       A^T[16] = [ a00 a10 a20 a30
                   a01 a11 a21 a31
                   a02 a12 a22 a32
                   a03 a13 a23 a33 ]

    As matrix is row-major layout in RAM in C, so

    A^T[16] = { a00 a10 a20 a30  a01 a11 a21 a31  a02 a12 a22 a32  a03 a13 a23 a33 } 

    B is the same case, we omit rewrite here.


If we were told matrix A, B and out are specified in row-major order.
we call myGlMultMatrix function to do the math. 
This is common implementation you may seen on non-opengl matrix multiplication.

myGlMultMatrix(A, B, out); ------ (1)

if we were told matrix A, B and out are specified in column-major order.
we have to transpose the matrix, reverse the input argument order, 
call the SAME function, that is:


myGlMultMatrix(B^T, A^T, out); ------ (2)

this is protype of myGlMultMatrix function:

But what happens if your were told that matrix A, B and out are specified in column-major order ?

This is endless confusing, we know that C program langurage array is row-major layout,
you can not change. Its actually means that you simplay transpose Matrix A and transpose
matrix B and reverse the order they are passed to myGlMultMatrix function:
the data still row-major in the perspective of C compiler.

let us explain it mathmatically:

`out = A x B` actually is equivalent to `out^T = B^T x A^T` where `^T` means matrix transpose



```
// A, B is raw-major order, call myGlMultMatrix(A, B, out1);
// then out1 is raw-major order.

// if A, B is column-major order, then both B^T and A^T are row-major,
// call myGlMultMatrix(B^T, A^T, out2); out2 is column-major order.

// out1 = A * B 
// out2 = B^T * A^T = (A * B)^T = out1^T;


void myGlMultMatrix(const float A[16], const float B[16], float out[16])
{
	int	i, j;

	for ( i = 0 ; i < 4 ; i++ )
    {
		for ( j = 0 ; j < 4 ; j++ )
        {
			out[ i * 4 + j ] =
				  A [ i * 4 + 0 ] * B [ 0 * 4 + j ]
				+ A [ i * 4 + 1 ] * B [ 1 * 4 + j ]
				+ A [ i * 4 + 2 ] * B [ 2 * 4 + j ]
				+ A [ i * 4 + 3 ] * B [ 3 * 4 + j ];
		}
	}
}


void MatrixMultiply4x4(const float A[16], const float B[16], float out[16])
{
    out[0] = A[0]*B[0] + A[1]*B[4] + A[2]*B[8] + A[3]*B[12];
    out[1] = A[0]*B[1] + A[1]*B[5] + A[2]*B[9] + A[3]*B[13];
    out[2] = A[0]*B[2] + A[1]*B[6] + A[2]*B[10] + A[3]*B[14];
    out[3] = A[0]*B[3] + A[1]*B[7] + A[2]*B[11] + A[3]*B[15];

    out[4] = A[4]*B[0] + A[5]*B[4] + A[6]*B[8] + A[7]*B[12];
    out[5] = A[4]*B[1] + A[5]*B[5] + A[6]*B[9] + A[7]*B[13];
    out[6] = A[4]*B[2] + A[5]*B[6] + A[6]*B[10] + A[7]*B[14];
    out[7] = A[4]*B[3] + A[5]*B[7] + A[6]*B[11] + A[7]*B[15];

    out[8] = A[8]*B[0] + A[9]*B[4] + A[10]*B[8] + A[11]*B[12];
    out[9] = A[8]*B[1] + A[9]*B[5] + A[10]*B[9] + A[11]*B[13];
    out[10] = A[8]*B[2] + A[9]*B[6] + A[10]*B[10] + A[11]*B[14];
    out[11] = A[8]*B[3] + A[9]*B[7] + A[10]*B[11] + A[11]*B[15];

    out[12] = A[12]*B[0] + A[13]*B[4] + A[14]*B[8] + A[15]*B[12];
    out[13] = A[12]*B[1] + A[13]*B[5] + A[14]*B[9] + A[15]*B[13];
    out[14] = A[12]*B[2] + A[13]*B[6] + A[14]*B[10] + A[15]*B[14];
    out[15] = A[12]*B[3] + A[13]*B[7] + A[14]*B[11] + A[15]*B[15];
}
```
