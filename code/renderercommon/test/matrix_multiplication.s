	.file	"matrix_multiplication.c"
	.text
	.p2align 4,,15
	.globl	Mat4Identity
	.type	Mat4Identity, @function
Mat4Identity:
.LFB570:
	.cfi_startproc
	movdqa	s_Identity4x4(%rip), %xmm0
	movdqa	16+s_Identity4x4(%rip), %xmm1
	movdqa	32+s_Identity4x4(%rip), %xmm2
	movdqa	48+s_Identity4x4(%rip), %xmm3
	movups	%xmm0, (%rdi)
	movups	%xmm1, 16(%rdi)
	movups	%xmm2, 32(%rdi)
	movups	%xmm3, 48(%rdi)
	ret
	.cfi_endproc
.LFE570:
	.size	Mat4Identity, .-Mat4Identity
	.p2align 4,,15
	.globl	Mat4Translation
	.type	Mat4Translation, @function
Mat4Translation:
.LFB571:
	.cfi_startproc
	movdqa	s_Identity4x4(%rip), %xmm0
	movdqa	16+s_Identity4x4(%rip), %xmm1
	movdqa	32+s_Identity4x4(%rip), %xmm2
	movdqa	48+s_Identity4x4(%rip), %xmm3
	movups	%xmm0, (%rsi)
	movups	%xmm3, 48(%rsi)
	movups	%xmm1, 16(%rsi)
	movups	%xmm2, 32(%rsi)
	movss	(%rdi), %xmm4
	movss	%xmm4, 48(%rsi)
	movss	4(%rdi), %xmm5
	movss	%xmm5, 52(%rsi)
	movss	8(%rdi), %xmm6
	movss	%xmm6, 56(%rsi)
	ret
	.cfi_endproc
.LFE571:
	.size	Mat4Translation, .-Mat4Translation
	.p2align 4,,15
	.globl	myGlMultMatrix
	.type	myGlMultMatrix, @function
myGlMultMatrix:
.LFB572:
	.cfi_startproc
	leaq	64(%rdi), %rax
.L5:
	movss	48(%rsi), %xmm0
	addq	$16, %rdi
	addq	$16, %rdx
	movss	32(%rsi), %xmm1
	mulss	-4(%rdi), %xmm0
	movss	16(%rsi), %xmm3
	mulss	-8(%rdi), %xmm1
	movss	(%rsi), %xmm2
	mulss	-12(%rdi), %xmm3
	mulss	-16(%rdi), %xmm2
	addss	%xmm1, %xmm0
	addss	%xmm2, %xmm3
	addss	%xmm3, %xmm0
	movss	%xmm0, -16(%rdx)
	movss	20(%rsi), %xmm4
	movss	4(%rsi), %xmm5
	mulss	-12(%rdi), %xmm4
	movss	52(%rsi), %xmm6
	mulss	-16(%rdi), %xmm5
	movss	36(%rsi), %xmm7
	mulss	-4(%rdi), %xmm6
	mulss	-8(%rdi), %xmm7
	addss	%xmm5, %xmm4
	addss	%xmm7, %xmm6
	addss	%xmm6, %xmm4
	movss	%xmm4, -12(%rdx)
	movss	56(%rsi), %xmm8
	movss	40(%rsi), %xmm9
	mulss	-4(%rdi), %xmm8
	movss	8(%rsi), %xmm10
	mulss	-8(%rdi), %xmm9
	movss	24(%rsi), %xmm11
	mulss	-16(%rdi), %xmm10
	mulss	-12(%rdi), %xmm11
	addss	%xmm9, %xmm8
	addss	%xmm11, %xmm10
	addss	%xmm10, %xmm8
	movss	%xmm8, -8(%rdx)
	movss	-16(%rdi), %xmm12
	movss	-12(%rdi), %xmm13
	mulss	12(%rsi), %xmm12
	movss	-8(%rdi), %xmm14
	mulss	28(%rsi), %xmm13
	mulss	44(%rsi), %xmm14
	movss	-4(%rdi), %xmm15
	mulss	60(%rsi), %xmm15
	addss	%xmm13, %xmm12
	addss	%xmm15, %xmm14
	addss	%xmm14, %xmm12
	movss	%xmm12, -4(%rdx)
	cmpq	%rax, %rdi
	jne	.L5
	rep ret
	.cfi_endproc
.LFE572:
	.size	myGlMultMatrix, .-myGlMultMatrix
	.p2align 4,,15
	.globl	MatrixMultiply4x4
	.type	MatrixMultiply4x4, @function
MatrixMultiply4x4:
.LFB573:
	.cfi_startproc
	movss	(%rdi), %xmm0
	movss	4(%rdi), %xmm1
	mulss	(%rsi), %xmm0
	movss	8(%rdi), %xmm3
	mulss	16(%rsi), %xmm1
	movss	12(%rdi), %xmm2
	mulss	32(%rsi), %xmm3
	mulss	48(%rsi), %xmm2
	addss	%xmm1, %xmm0
	addss	%xmm2, %xmm3
	addss	%xmm3, %xmm0
	movss	%xmm0, (%rdx)
	movss	(%rdi), %xmm4
	movss	4(%rdi), %xmm5
	mulss	4(%rsi), %xmm4
	movss	8(%rdi), %xmm6
	mulss	20(%rsi), %xmm5
	movss	12(%rdi), %xmm7
	mulss	36(%rsi), %xmm6
	mulss	52(%rsi), %xmm7
	addss	%xmm5, %xmm4
	addss	%xmm7, %xmm6
	addss	%xmm6, %xmm4
	movss	%xmm4, 4(%rdx)
	movss	(%rdi), %xmm8
	movss	4(%rdi), %xmm9
	mulss	8(%rsi), %xmm8
	movss	8(%rdi), %xmm10
	mulss	24(%rsi), %xmm9
	movss	12(%rdi), %xmm11
	mulss	40(%rsi), %xmm10
	mulss	56(%rsi), %xmm11
	addss	%xmm9, %xmm8
	addss	%xmm11, %xmm10
	addss	%xmm10, %xmm8
	movss	%xmm8, 8(%rdx)
	movss	(%rdi), %xmm12
	movss	4(%rdi), %xmm13
	mulss	12(%rsi), %xmm12
	movss	8(%rdi), %xmm14
	mulss	28(%rsi), %xmm13
	mulss	44(%rsi), %xmm14
	movss	12(%rdi), %xmm15
	mulss	60(%rsi), %xmm15
	addss	%xmm13, %xmm12
	addss	%xmm15, %xmm14
	addss	%xmm14, %xmm12
	movss	%xmm12, 12(%rdx)
	movss	16(%rdi), %xmm0
	movss	20(%rdi), %xmm1
	mulss	(%rsi), %xmm0
	movss	24(%rdi), %xmm3
	mulss	16(%rsi), %xmm1
	movss	28(%rdi), %xmm2
	mulss	32(%rsi), %xmm3
	mulss	48(%rsi), %xmm2
	addss	%xmm1, %xmm0
	addss	%xmm2, %xmm3
	addss	%xmm3, %xmm0
	movss	%xmm0, 16(%rdx)
	movss	16(%rdi), %xmm4
	movss	20(%rdi), %xmm5
	mulss	4(%rsi), %xmm4
	movss	24(%rdi), %xmm6
	mulss	20(%rsi), %xmm5
	movss	28(%rdi), %xmm7
	mulss	36(%rsi), %xmm6
	mulss	52(%rsi), %xmm7
	addss	%xmm5, %xmm4
	addss	%xmm7, %xmm6
	addss	%xmm6, %xmm4
	movss	%xmm4, 20(%rdx)
	movss	16(%rdi), %xmm8
	movss	20(%rdi), %xmm9
	mulss	8(%rsi), %xmm8
	movss	24(%rdi), %xmm10
	mulss	24(%rsi), %xmm9
	movss	28(%rdi), %xmm11
	mulss	40(%rsi), %xmm10
	mulss	56(%rsi), %xmm11
	addss	%xmm9, %xmm8
	addss	%xmm11, %xmm10
	addss	%xmm10, %xmm8
	movss	%xmm8, 24(%rdx)
	movss	16(%rdi), %xmm12
	mulss	12(%rsi), %xmm12
	movss	20(%rdi), %xmm13
	movss	24(%rdi), %xmm14
	mulss	28(%rsi), %xmm13
	movss	28(%rdi), %xmm15
	mulss	44(%rsi), %xmm14
	mulss	60(%rsi), %xmm15
	addss	%xmm13, %xmm12
	addss	%xmm15, %xmm14
	addss	%xmm14, %xmm12
	movss	%xmm12, 28(%rdx)
	movss	32(%rdi), %xmm0
	movss	36(%rdi), %xmm1
	mulss	(%rsi), %xmm0
	movss	40(%rdi), %xmm3
	mulss	16(%rsi), %xmm1
	movss	44(%rdi), %xmm2
	mulss	32(%rsi), %xmm3
	mulss	48(%rsi), %xmm2
	addss	%xmm1, %xmm0
	addss	%xmm2, %xmm3
	addss	%xmm3, %xmm0
	movss	%xmm0, 32(%rdx)
	movss	32(%rdi), %xmm4
	movss	36(%rdi), %xmm5
	mulss	4(%rsi), %xmm4
	movss	40(%rdi), %xmm6
	mulss	20(%rsi), %xmm5
	movss	44(%rdi), %xmm7
	mulss	36(%rsi), %xmm6
	mulss	52(%rsi), %xmm7
	addss	%xmm5, %xmm4
	addss	%xmm7, %xmm6
	addss	%xmm6, %xmm4
	movss	%xmm4, 36(%rdx)
	movss	32(%rdi), %xmm8
	movss	36(%rdi), %xmm9
	mulss	8(%rsi), %xmm8
	movss	40(%rdi), %xmm10
	mulss	24(%rsi), %xmm9
	mulss	40(%rsi), %xmm10
	movss	44(%rdi), %xmm11
	mulss	56(%rsi), %xmm11
	addss	%xmm9, %xmm8
	addss	%xmm11, %xmm10
	addss	%xmm10, %xmm8
	movss	%xmm8, 40(%rdx)
	movss	32(%rdi), %xmm12
	movss	36(%rdi), %xmm13
	mulss	12(%rsi), %xmm12
	movss	40(%rdi), %xmm14
	mulss	28(%rsi), %xmm13
	movss	44(%rdi), %xmm15
	mulss	44(%rsi), %xmm14
	mulss	60(%rsi), %xmm15
	addss	%xmm13, %xmm12
	addss	%xmm15, %xmm14
	addss	%xmm14, %xmm12
	movss	%xmm12, 44(%rdx)
	movss	48(%rdi), %xmm0
	movss	52(%rdi), %xmm1
	mulss	(%rsi), %xmm0
	movss	56(%rdi), %xmm3
	mulss	16(%rsi), %xmm1
	movss	60(%rdi), %xmm2
	mulss	32(%rsi), %xmm3
	mulss	48(%rsi), %xmm2
	addss	%xmm1, %xmm0
	addss	%xmm2, %xmm3
	addss	%xmm3, %xmm0
	movss	%xmm0, 48(%rdx)
	movss	48(%rdi), %xmm4
	movss	52(%rdi), %xmm5
	mulss	4(%rsi), %xmm4
	movss	56(%rdi), %xmm6
	mulss	20(%rsi), %xmm5
	movss	60(%rdi), %xmm7
	mulss	36(%rsi), %xmm6
	mulss	52(%rsi), %xmm7
	addss	%xmm5, %xmm4
	addss	%xmm7, %xmm6
	addss	%xmm6, %xmm4
	movss	%xmm4, 52(%rdx)
	movss	48(%rdi), %xmm8
	mulss	8(%rsi), %xmm8
	movss	52(%rdi), %xmm9
	movss	56(%rdi), %xmm10
	mulss	24(%rsi), %xmm9
	movss	60(%rdi), %xmm11
	mulss	40(%rsi), %xmm10
	mulss	56(%rsi), %xmm11
	addss	%xmm9, %xmm8
	addss	%xmm11, %xmm10
	addss	%xmm10, %xmm8
	movss	%xmm8, 56(%rdx)
	movss	48(%rdi), %xmm12
	movss	52(%rdi), %xmm13
	mulss	12(%rsi), %xmm12
	movss	56(%rdi), %xmm14
	mulss	28(%rsi), %xmm13
	movss	60(%rdi), %xmm15
	mulss	44(%rsi), %xmm14
	mulss	60(%rsi), %xmm15
	addss	%xmm13, %xmm12
	addss	%xmm15, %xmm14
	addss	%xmm14, %xmm12
	movss	%xmm12, 60(%rdx)
	ret
	.cfi_endproc
.LFE573:
	.size	MatrixMultiply4x4, .-MatrixMultiply4x4
	.p2align 4,,15
	.globl	MatrixMultiply4x4_SSE
	.type	MatrixMultiply4x4_SSE, @function
MatrixMultiply4x4_SSE:
.LFB574:
	.cfi_startproc
	movss	8(%rdi), %xmm1
	movss	12(%rdi), %xmm0
	shufps	$0, %xmm1, %xmm1
	shufps	$0, %xmm0, %xmm0
	movaps	48(%rsi), %xmm5
	movaps	32(%rsi), %xmm4
	mulps	%xmm5, %xmm0
	movss	4(%rdi), %xmm6
	mulps	%xmm4, %xmm1
	movaps	16(%rsi), %xmm2
	shufps	$0, %xmm6, %xmm6
	movss	(%rdi), %xmm7
	shufps	$0, %xmm7, %xmm7
	movaps	(%rsi), %xmm3
	addps	%xmm0, %xmm1
	mulps	%xmm2, %xmm6
	mulps	%xmm3, %xmm7
	addps	%xmm6, %xmm1
	addps	%xmm7, %xmm1
	movaps	%xmm1, (%rdx)
	movss	28(%rdi), %xmm8
	movss	24(%rdi), %xmm9
	shufps	$0, %xmm8, %xmm8
	shufps	$0, %xmm9, %xmm9
	movss	16(%rdi), %xmm10
	mulps	%xmm5, %xmm8
	movss	20(%rdi), %xmm11
	mulps	%xmm4, %xmm9
	shufps	$0, %xmm10, %xmm10
	shufps	$0, %xmm11, %xmm11
	mulps	%xmm3, %xmm10
	addps	%xmm9, %xmm8
	mulps	%xmm2, %xmm11
	addps	%xmm10, %xmm8
	addps	%xmm11, %xmm8
	movaps	%xmm8, 16(%rdx)
	movss	44(%rdi), %xmm12
	movss	40(%rdi), %xmm13
	shufps	$0, %xmm12, %xmm12
	shufps	$0, %xmm13, %xmm13
	movss	32(%rdi), %xmm14
	mulps	%xmm5, %xmm12
	movss	36(%rdi), %xmm15
	mulps	%xmm4, %xmm13
	shufps	$0, %xmm14, %xmm14
	shufps	$0, %xmm15, %xmm15
	mulps	%xmm3, %xmm14
	addps	%xmm13, %xmm12
	mulps	%xmm2, %xmm15
	addps	%xmm14, %xmm12
	addps	%xmm15, %xmm12
	movaps	%xmm12, 32(%rdx)
	movss	60(%rdi), %xmm0
	shufps	$0, %xmm0, %xmm0
	mulps	%xmm5, %xmm0
	movss	56(%rdi), %xmm5
	shufps	$0, %xmm5, %xmm5
	mulps	%xmm5, %xmm4
	addps	%xmm4, %xmm0
	movss	48(%rdi), %xmm4
	shufps	$0, %xmm4, %xmm4
	mulps	%xmm4, %xmm3
	addps	%xmm3, %xmm0
	movss	52(%rdi), %xmm3
	shufps	$0, %xmm3, %xmm3
	mulps	%xmm3, %xmm2
	addps	%xmm2, %xmm0
	movaps	%xmm0, 48(%rdx)
	ret
	.cfi_endproc
.LFE574:
	.size	MatrixMultiply4x4_SSE, .-MatrixMultiply4x4_SSE
	.p2align 4,,15
	.globl	Mat4Transform
	.type	Mat4Transform, @function
Mat4Transform:
.LFB575:
	.cfi_startproc
	movss	(%rsi), %xmm0
	movss	4(%rsi), %xmm3
	movss	8(%rsi), %xmm1
	movss	12(%rsi), %xmm2
	movss	(%rdi), %xmm4
	movss	16(%rdi), %xmm5
	mulss	%xmm0, %xmm4
	movss	32(%rdi), %xmm7
	mulss	%xmm3, %xmm5
	movss	48(%rdi), %xmm6
	mulss	%xmm1, %xmm7
	mulss	%xmm2, %xmm6
	addss	%xmm5, %xmm4
	addss	%xmm6, %xmm7
	addss	%xmm7, %xmm4
	movss	%xmm4, (%rdx)
	movss	4(%rdi), %xmm8
	movss	20(%rdi), %xmm9
	mulss	%xmm0, %xmm8
	movss	36(%rdi), %xmm10
	mulss	%xmm3, %xmm9
	movss	52(%rdi), %xmm11
	mulss	%xmm1, %xmm10
	mulss	%xmm2, %xmm11
	addss	%xmm9, %xmm8
	addss	%xmm11, %xmm10
	addss	%xmm10, %xmm8
	movss	%xmm8, 4(%rdx)
	movss	8(%rdi), %xmm12
	movss	24(%rdi), %xmm13
	mulss	%xmm0, %xmm12
	movss	40(%rdi), %xmm14
	mulss	%xmm3, %xmm13
	movss	56(%rdi), %xmm15
	mulss	%xmm1, %xmm14
	mulss	%xmm2, %xmm15
	addss	%xmm13, %xmm12
	addss	%xmm15, %xmm14
	addss	%xmm14, %xmm12
	movss	%xmm12, 8(%rdx)
	mulss	12(%rdi), %xmm0
	mulss	28(%rdi), %xmm3
	mulss	44(%rdi), %xmm1
	mulss	60(%rdi), %xmm2
	addss	%xmm3, %xmm0
	addss	%xmm2, %xmm1
	addss	%xmm1, %xmm0
	movss	%xmm0, 12(%rdx)
	ret
	.cfi_endproc
.LFE575:
	.size	Mat4Transform, .-Mat4Transform
	.p2align 4,,15
	.globl	Mat4x1Transform_SSE
	.type	Mat4x1Transform_SSE, @function
Mat4x1Transform_SSE:
.LFB576:
	.cfi_startproc
	movss	12(%rsi), %xmm0
	movss	8(%rsi), %xmm1
	shufps	$0, %xmm0, %xmm0
	shufps	$0, %xmm1, %xmm1
	movss	4(%rsi), %xmm2
	mulps	48(%rdi), %xmm0
	movss	(%rsi), %xmm3
	mulps	32(%rdi), %xmm1
	shufps	$0, %xmm2, %xmm2
	shufps	$0, %xmm3, %xmm3
	mulps	16(%rdi), %xmm2
	addps	%xmm1, %xmm0
	mulps	(%rdi), %xmm3
	addps	%xmm2, %xmm0
	addps	%xmm3, %xmm0
	movaps	%xmm0, (%rdx)
	ret
	.cfi_endproc
.LFE576:
	.size	Mat4x1Transform_SSE, .-Mat4x1Transform_SSE
	.p2align 4,,15
	.globl	Mat3x3Identity
	.type	Mat3x3Identity, @function
Mat3x3Identity:
.LFB577:
	.cfi_startproc
	movl	32+s_Identity3x3(%rip), %eax
	movdqa	s_Identity3x3(%rip), %xmm0
	movdqa	16+s_Identity3x3(%rip), %xmm1
	movl	%eax, 32(%rdi)
	movups	%xmm0, (%rdi)
	movups	%xmm1, 16(%rdi)
	ret
	.cfi_endproc
.LFE577:
	.size	Mat3x3Identity, .-Mat3x3Identity
	.p2align 4,,15
	.globl	Mat4Ortho
	.type	Mat4Ortho, @function
Mat4Ortho:
.LFB578:
	.cfi_startproc
	movaps	%xmm1, %xmm7
	movq	$0, 4(%rdi)
	movss	.LC0(%rip), %xmm6
	addss	%xmm0, %xmm1
	subss	%xmm0, %xmm7
	movaps	%xmm6, %xmm9
	movaps	%xmm3, %xmm10
	movss	.LC1(%rip), %xmm0
	movaps	%xmm5, %xmm12
	movq	$0, 12(%rdi)
	subss	%xmm2, %xmm10
	movaps	%xmm6, %xmm8
	divss	%xmm7, %xmm9
	subss	%xmm4, %xmm12
	movq	$0, 24(%rdi)
	movaps	%xmm6, %xmm7
	movq	$0, 32(%rdi)
	addss	%xmm2, %xmm3
	movl	$0x00000000, 44(%rdi)
	movss	%xmm6, 60(%rdi)
	addss	%xmm4, %xmm5
	divss	%xmm10, %xmm8
	movaps	%xmm9, %xmm13
	addss	%xmm9, %xmm13
	movss	%xmm13, (%rdi)
	divss	%xmm12, %xmm7
	mulss	%xmm9, %xmm1
	mulss	%xmm8, %xmm3
	mulss	%xmm7, %xmm5
	movaps	%xmm7, %xmm2
	xorps	%xmm0, %xmm1
	addss	%xmm7, %xmm2
	xorps	%xmm0, %xmm3
	movss	%xmm1, 48(%rdi)
	movaps	%xmm8, %xmm1
	xorps	%xmm0, %xmm5
	movss	%xmm3, 52(%rdi)
	addss	%xmm8, %xmm1
	movss	%xmm2, 40(%rdi)
	movss	%xmm5, 56(%rdi)
	movss	%xmm1, 20(%rdi)
	ret
	.cfi_endproc
.LFE578:
	.size	Mat4Ortho, .-Mat4Ortho
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC3:
	.string	"\n float %s[3] = {%f, %f, %f};\n"
	.text
	.p2align 4,,15
	.globl	print3f
	.type	print3f, @function
print3f:
.LFB579:
	.cfi_startproc
	pxor	%xmm0, %xmm0
	movq	%rdi, %rdx
	pxor	%xmm2, %xmm2
	movl	$1, %edi
	pxor	%xmm1, %xmm1
	movl	$3, %eax
	cvtss2sd	(%rsi), %xmm0
	cvtss2sd	8(%rsi), %xmm2
	cvtss2sd	4(%rsi), %xmm1
	leaq	.LC3(%rip), %rsi
	jmp	__printf_chk@PLT
	.cfi_endproc
.LFE579:
	.size	print3f, .-print3f
	.section	.rodata.str1.8
	.align 8
.LC4:
	.string	"\n float %s[4] = {%f, %f, %f, %f};\n"
	.text
	.p2align 4,,15
	.globl	print4f
	.type	print4f, @function
print4f:
.LFB580:
	.cfi_startproc
	pxor	%xmm0, %xmm0
	movq	%rdi, %rdx
	pxor	%xmm3, %xmm3
	movl	$1, %edi
	pxor	%xmm2, %xmm2
	movl	$4, %eax
	pxor	%xmm1, %xmm1
	cvtss2sd	(%rsi), %xmm0
	cvtss2sd	12(%rsi), %xmm3
	cvtss2sd	8(%rsi), %xmm2
	cvtss2sd	4(%rsi), %xmm1
	leaq	.LC4(%rip), %rsi
	jmp	__printf_chk@PLT
	.cfi_endproc
.LFE580:
	.size	print4f, .-print4f
	.section	.rodata.str1.8
	.align 8
.LC5:
	.string	"\n float %s[16] = {%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f};\n"
	.text
	.p2align 4,,15
	.globl	printMat4x4f
	.type	printMat4x4f, @function
printMat4x4f:
.LFB581:
	.cfi_startproc
	pxor	%xmm1, %xmm1
	subq	$72, %rsp
	.cfi_def_cfa_offset 80
	pxor	%xmm0, %xmm0
	movq	%rdi, %rdx
	pxor	%xmm2, %xmm2
	movl	$1, %edi
	cvtss2sd	60(%rsi), %xmm1
	pxor	%xmm3, %xmm3
	cvtss2sd	(%rsi), %xmm0
	pxor	%xmm4, %xmm4
	movsd	%xmm1, 56(%rsp)
	pxor	%xmm5, %xmm5
	pxor	%xmm6, %xmm6
	movl	$8, %eax
	cvtss2sd	56(%rsi), %xmm2
	pxor	%xmm7, %xmm7
	pxor	%xmm8, %xmm8
	movsd	%xmm2, 48(%rsp)
	pxor	%xmm1, %xmm1
	pxor	%xmm2, %xmm2
	cvtss2sd	52(%rsi), %xmm3
	movsd	%xmm3, 40(%rsp)
	pxor	%xmm3, %xmm3
	cvtss2sd	48(%rsi), %xmm4
	movsd	%xmm4, 32(%rsp)
	pxor	%xmm4, %xmm4
	cvtss2sd	44(%rsi), %xmm5
	movsd	%xmm5, 24(%rsp)
	pxor	%xmm5, %xmm5
	cvtss2sd	40(%rsi), %xmm6
	movsd	%xmm6, 16(%rsp)
	pxor	%xmm6, %xmm6
	cvtss2sd	36(%rsi), %xmm7
	movsd	%xmm7, 8(%rsp)
	pxor	%xmm7, %xmm7
	cvtss2sd	32(%rsi), %xmm8
	movsd	%xmm8, (%rsp)
	cvtss2sd	28(%rsi), %xmm7
	cvtss2sd	24(%rsi), %xmm6
	cvtss2sd	20(%rsi), %xmm5
	cvtss2sd	16(%rsi), %xmm4
	cvtss2sd	12(%rsi), %xmm3
	cvtss2sd	8(%rsi), %xmm2
	cvtss2sd	4(%rsi), %xmm1
	leaq	.LC5(%rip), %rsi
	call	__printf_chk@PLT
	addq	$72, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE581:
	.size	printMat4x4f, .-printMat4x4f
	.p2align 4,,15
	.globl	TransformModelToClip_SSE
	.type	TransformModelToClip_SSE, @function
TransformModelToClip_SSE:
.LFB582:
	.cfi_startproc
	movss	44(%rsi), %xmm0
	movss	40(%rsi), %xmm1
	shufps	$0, %xmm0, %xmm0
	shufps	$0, %xmm1, %xmm1
	movaps	32(%rdx), %xmm6
	movaps	48(%rdx), %xmm5
	mulps	%xmm6, %xmm1
	movss	32(%rsi), %xmm2
	mulps	%xmm5, %xmm0
	movaps	(%rdx), %xmm4
	shufps	$0, %xmm2, %xmm2
	movss	36(%rsi), %xmm7
	shufps	$0, %xmm7, %xmm7
	movaps	16(%rdx), %xmm3
	mulps	%xmm4, %xmm2
	addps	%xmm1, %xmm0
	movss	56(%rsi), %xmm10
	mulps	%xmm3, %xmm7
	movss	24(%rsi), %xmm14
	shufps	$0, %xmm10, %xmm10
	movss	60(%rsi), %xmm9
	shufps	$0, %xmm14, %xmm14
	addps	%xmm2, %xmm0
	movss	28(%rsi), %xmm13
	mulps	%xmm6, %xmm10
	movss	48(%rsi), %xmm11
	mulps	%xmm6, %xmm14
	movss	16(%rsi), %xmm15
	movss	52(%rsi), %xmm12
	addps	%xmm7, %xmm0
	movss	8(%rsi), %xmm7
	shufps	$0, %xmm7, %xmm7
	shufps	$0, %xmm9, %xmm9
	movss	20(%rsi), %xmm1
	shufps	$0, %xmm13, %xmm13
	mulps	%xmm6, %xmm7
	movss	12(%rsi), %xmm6
	movss	8(%rdi), %xmm8
	shufps	$0, %xmm6, %xmm6
	movss	4(%rdi), %xmm2
	mulps	%xmm5, %xmm9
	mulps	%xmm5, %xmm13
	mulps	%xmm6, %xmm5
	addps	%xmm10, %xmm9
	shufps	$0, %xmm11, %xmm11
	shufps	$0, %xmm15, %xmm15
	addps	%xmm14, %xmm13
	shufps	$0, %xmm12, %xmm12
	addps	%xmm5, %xmm7
	movss	(%rsi), %xmm5
	shufps	$0, %xmm5, %xmm5
	mulps	%xmm4, %xmm11
	mulps	%xmm4, %xmm15
	mulps	%xmm5, %xmm4
	addps	%xmm11, %xmm9
	shufps	$0, %xmm1, %xmm1
	mulps	%xmm3, %xmm12
	addps	%xmm15, %xmm13
	mulps	%xmm3, %xmm1
	addps	%xmm4, %xmm7
	movss	4(%rsi), %xmm4
	shufps	$0, %xmm8, %xmm8
	shufps	$0, %xmm4, %xmm4
	addps	%xmm12, %xmm9
	shufps	$0, %xmm2, %xmm2
	mulps	%xmm8, %xmm0
	addps	%xmm1, %xmm13
	mulps	%xmm4, %xmm3
	addps	%xmm9, %xmm0
	mulps	%xmm2, %xmm13
	addps	%xmm3, %xmm7
	movss	(%rdi), %xmm3
	shufps	$0, %xmm3, %xmm3
	addps	%xmm13, %xmm0
	mulps	%xmm3, %xmm7
	addps	%xmm0, %xmm7
	movaps	%xmm7, (%rcx)
	ret
	.cfi_endproc
.LFE582:
	.size	TransformModelToClip_SSE, .-TransformModelToClip_SSE
	.p2align 4,,15
	.globl	TransformModelToClip_SSE2
	.type	TransformModelToClip_SSE2, @function
TransformModelToClip_SSE2:
.LFB583:
	.cfi_startproc
	movss	8(%rdi), %xmm0
	movss	4(%rdi), %xmm1
	shufps	$0, %xmm0, %xmm0
	shufps	$0, %xmm1, %xmm1
	movss	(%rdi), %xmm2
	mulps	32(%rsi), %xmm0
	mulps	16(%rsi), %xmm1
	shufps	$0, %xmm2, %xmm2
	mulps	(%rsi), %xmm2
	addps	%xmm1, %xmm0
	addps	48(%rsi), %xmm0
	addps	%xmm2, %xmm0
	movaps	%xmm0, %xmm3
	movaps	%xmm0, %xmm4
	movaps	%xmm0, %xmm5
	shufps	$255, %xmm0, %xmm3
	shufps	$170, %xmm0, %xmm4
	shufps	$85, %xmm0, %xmm5
	mulps	48(%rdx), %xmm3
	mulps	32(%rdx), %xmm4
	mulps	16(%rdx), %xmm5
	shufps	$0, %xmm0, %xmm0
	addps	%xmm4, %xmm3
	mulps	(%rdx), %xmm0
	addps	%xmm5, %xmm3
	addps	%xmm3, %xmm0
	movaps	%xmm0, (%rcx)
	ret
	.cfi_endproc
.LFE583:
	.size	TransformModelToClip_SSE2, .-TransformModelToClip_SSE2
	.p2align 4,,15
	.globl	TransformModelToClip_o
	.type	TransformModelToClip_o, @function
TransformModelToClip_o:
.LFB584:
	.cfi_startproc
	movss	(%rdi), %xmm2
	movss	(%rsi), %xmm1
	movss	4(%rsi), %xmm8
	mulss	%xmm2, %xmm1
	movss	8(%rsi), %xmm12
	mulss	%xmm2, %xmm8
	mulss	%xmm2, %xmm12
	movss	4(%rdi), %xmm3
	mulss	12(%rsi), %xmm2
	movss	8(%rdi), %xmm7
	movss	16(%rsi), %xmm5
	movss	32(%rsi), %xmm0
	mulss	%xmm3, %xmm5
	addss	52(%rsi), %xmm8
	movss	20(%rsi), %xmm6
	mulss	%xmm7, %xmm0
	movss	36(%rsi), %xmm4
	mulss	%xmm3, %xmm6
	movss	24(%rsi), %xmm10
	mulss	%xmm7, %xmm4
	movss	40(%rsi), %xmm11
	mulss	%xmm3, %xmm10
	mulss	%xmm7, %xmm11
	addss	60(%rsi), %xmm2
	mulss	28(%rsi), %xmm3
	addss	48(%rsi), %xmm0
	mulss	44(%rsi), %xmm7
	addss	56(%rsi), %xmm12
	addss	%xmm5, %xmm1
	movss	16(%rdx), %xmm15
	addss	%xmm11, %xmm10
	addss	%xmm4, %xmm6
	movaps	%xmm1, %xmm9
	addss	%xmm7, %xmm3
	movaps	%xmm10, %xmm13
	addss	%xmm0, %xmm9
	movss	48(%rdx), %xmm1
	addss	%xmm8, %xmm6
	addss	%xmm12, %xmm13
	addss	%xmm2, %xmm3
	movss	(%rdx), %xmm2
	mulss	%xmm9, %xmm2
	mulss	%xmm6, %xmm15
	movaps	%xmm3, %xmm14
	mulss	%xmm3, %xmm1
	movss	32(%rdx), %xmm3
	mulss	%xmm13, %xmm3
	addss	%xmm2, %xmm15
	addss	%xmm3, %xmm1
	addss	%xmm1, %xmm15
	movss	%xmm15, (%rcx)
	movss	20(%rdx), %xmm5
	movss	4(%rdx), %xmm0
	mulss	%xmm6, %xmm5
	movss	52(%rdx), %xmm4
	mulss	%xmm9, %xmm0
	movss	36(%rdx), %xmm8
	mulss	%xmm14, %xmm4
	mulss	%xmm13, %xmm8
	addss	%xmm0, %xmm5
	addss	%xmm8, %xmm4
	addss	%xmm4, %xmm5
	movss	%xmm5, 4(%rcx)
	movss	24(%rdx), %xmm10
	movss	8(%rdx), %xmm11
	mulss	%xmm6, %xmm10
	movss	56(%rdx), %xmm12
	mulss	%xmm9, %xmm11
	movss	40(%rdx), %xmm7
	mulss	%xmm14, %xmm12
	mulss	%xmm13, %xmm7
	addss	%xmm11, %xmm10
	addss	%xmm7, %xmm12
	addss	%xmm12, %xmm10
	movss	%xmm10, 8(%rcx)
	mulss	44(%rdx), %xmm13
	mulss	60(%rdx), %xmm14
	mulss	12(%rdx), %xmm9
	mulss	28(%rdx), %xmm6
	addss	%xmm13, %xmm14
	addss	%xmm6, %xmm9
	addss	%xmm9, %xmm14
	movss	%xmm14, 12(%rcx)
	ret
	.cfi_endproc
.LFE584:
	.size	TransformModelToClip_o, .-TransformModelToClip_o
	.section	.rodata
	.align 32
	.type	s_Identity4x4, @object
	.size	s_Identity4x4, 64
s_Identity4x4:
	.long	1065353216
	.long	0
	.long	0
	.long	0
	.long	0
	.long	1065353216
	.long	0
	.long	0
	.long	0
	.long	0
	.long	1065353216
	.long	0
	.long	0
	.long	0
	.long	0
	.long	1065353216
	.align 32
	.type	s_Identity3x3, @object
	.size	s_Identity3x3, 36
s_Identity3x3:
	.long	1065353216
	.long	0
	.long	0
	.long	0
	.long	1065353216
	.long	0
	.long	0
	.long	0
	.long	1065353216
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC0:
	.long	1065353216
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC1:
	.long	2147483648
	.long	0
	.long	0
	.long	0
	.ident	"GCC: (Ubuntu 7.3.0-27ubuntu1~18.04) 7.3.0"
	.section	.note.GNU-stack,"",@progbits
