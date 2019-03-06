	.file	"test_transform_model_to_clip.c"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	" %f,\t"
	.text
	.p2align 4,,15
	.globl	MM4x1Print
	.type	MM4x1Print, @function
MM4x1Print:
.LFB64:
	.cfi_startproc
	pushq	%r12
	.cfi_def_cfa_offset 16
	.cfi_offset 12, -16
	pushq	%rbp
	.cfi_def_cfa_offset 24
	.cfi_offset 6, -24
	leaq	.LC0(%rip), %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -32
	movq	%rdi, %rbx
	movl	$10, %edi
	leaq	16(%rbx), %r12
	call	putchar@PLT
.L2:
	pxor	%xmm0, %xmm0
	movq	%rbp, %rsi
	movl	$1, %edi
	movl	$1, %eax
	addq	$4, %rbx
	cvtss2sd	-4(%rbx), %xmm0
	call	__printf_chk@PLT
	cmpq	%r12, %rbx
	jne	.L2
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	movl	$10, %edi
	jmp	putchar@PLT
	.cfi_endproc
.LFE64:
	.size	MM4x1Print, .-MM4x1Print
	.section	.rodata.str1.1
.LC3:
	.string	"out2"
.LC4:
	.string	"TransformModelToClip_o: %d\n"
.LC5:
	.string	"out1"
.LC6:
	.string	"TransformModelToClip_SSE: %d\n"
.LC7:
	.string	"dst"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC8:
	.string	"TransformModelToClip_SSE2: %d\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB65:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movabsq	$-4326937629098328449, %rdi
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movabsq	$4864956186555932116, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movabsq	$4575657222557755428, %rbp
	pxor	%xmm0, %xmm0
	movabsq	$-4886452575131449585, %rbx
	subq	$264, %rsp
	.cfi_def_cfa_offset 320
	movabsq	$4573736238159794255, %rsi
	movabsq	$4497579074244913108, %rdx
	movq	%rbx, 128(%rsp)
	movq	%rdi, 160(%rsp)
	leaq	80(%rsp), %r13
	movq	%rbp, 168(%rsp)
	movq	%r12, 104(%rsp)
	leaq	16(%rsp), %rdi
	leaq	176(%rsp), %rbx
	leaq	112(%rsp), %rbp
	leaq	48(%rsp), %r12
	movq	%fs:40, %rax
	movq	%rax, 248(%rsp)
	xorl	%eax, %eax
	movl	$3212342322, %ecx
	movabsq	$-4291367493778866176, %rax
	movq	%rsi, 144(%rsp)
	movabsq	$4550225070781366272, %r8
	movabsq	$-4647714812233413539, %r9
	movl	$3238053423, %r10d
	movabsq	$4801377175746775432, %r11
	xorl	%esi, %esi
	xorl	%r15d, %r15d
	movl	$-1431655765, %r14d
	movq	%rax, 48(%rsp)
	movl	$1134034944, 56(%rsp)
	movaps	%xmm0, 64(%rsp)
	movq	%rdx, 112(%rsp)
	movq	%rcx, 120(%rsp)
	movaps	%xmm0, 80(%rsp)
	movq	$1033383090, 136(%rsp)
	movq	$1047184699, 152(%rsp)
	movq	$1052400199, 176(%rsp)
	movq	$0, 184(%rsp)
	movq	%r8, 192(%rsp)
	movq	$0, 200(%rsp)
	movq	$0, 208(%rsp)
	movq	%r9, 216(%rsp)
	movq	$0, 224(%rsp)
	movq	%r10, 232(%rsp)
	movq	%r11, 96(%rsp)
	movq	%rdi, (%rsp)
	call	gettimeofday@PLT
	.p2align 4,,10
	.p2align 3
.L7:
	pxor	%xmm1, %xmm1
	movl	%r15d, %eax
	andl	$15, %eax
	pxor	%xmm4, %xmm4
	movslq	%eax, %rdx
	pxor	%xmm2, %xmm2
	cvtsi2ss	%eax, %xmm1
	sarl	%eax
	movl	%r15d, %r8d
	movl	%r15d, %esi
	cvtsi2sd	%eax, %xmm4
	andl	$3, %r8d
	pxor	%xmm3, %xmm3
	movl	%r15d, %eax
	pxor	%xmm5, %xmm5
	pxor	%xmm6, %xmm6
	cvtsi2ss	%r8d, %xmm6
	addsd	.LC2(%rip), %xmm4
	mulss	.LC1(%rip), %xmm1
	cvtsd2ss	%xmm4, %xmm5
	cvtss2sd	%xmm1, %xmm2
	mulss	.LC1(%rip), %xmm6
	subsd	.LC2(%rip), %xmm2
	cvtsd2ss	%xmm2, %xmm3
	movss	%xmm5, 176(%rsp,%rdx,4)
	movss	%xmm3, 112(%rsp,%rdx,4)
	mull	%r14d
	shrl	%edx
	leal	(%rdx,%rdx,2), %ecx
	movq	%rbx, %rdx
	subl	%ecx, %esi
	movq	%r13, %rcx
	movslq	%esi, %rdi
	movq	%rbp, %rsi
	movss	%xmm6, 48(%rsp,%rdi,4)
	movq	%r12, %rdi
	call	TransformModelToClip_o@PLT
	leal	1(%r15), %r9d
	pxor	%xmm7, %xmm7
	pxor	%xmm10, %xmm10
	addl	$2, %r15d
	movl	%r9d, %r10d
	pxor	%xmm12, %xmm12
	andl	$15, %r10d
	pxor	%xmm8, %xmm8
	cvtsi2ss	%r10d, %xmm7
	movslq	%r10d, %r11
	sarl	%r10d
	movl	%r9d, %eax
	cvtsi2sd	%r10d, %xmm10
	movl	%r9d, %esi
	andl	$3, %r9d
	pxor	%xmm9, %xmm9
	cvtsi2ss	%r9d, %xmm12
	mull	%r14d
	pxor	%xmm11, %xmm11
	shrl	%edx
	leal	(%rdx,%rdx,2), %ecx
	movq	%rbx, %rdx
	subl	%ecx, %esi
	movq	%r13, %rcx
	movslq	%esi, %rdi
	movq	%rbp, %rsi
	addsd	.LC2(%rip), %xmm10
	mulss	.LC1(%rip), %xmm7
	mulss	.LC1(%rip), %xmm12
	cvtsd2ss	%xmm10, %xmm11
	cvtss2sd	%xmm7, %xmm8
	subsd	.LC2(%rip), %xmm8
	movss	%xmm12, 48(%rsp,%rdi,4)
	movq	%r12, %rdi
	cvtsd2ss	%xmm8, %xmm9
	movss	%xmm11, 176(%rsp,%r11,4)
	movss	%xmm9, 112(%rsp,%r11,4)
	call	TransformModelToClip_o@PLT
	cmpl	$100000000, %r15d
	jne	.L7
	leaq	32(%rsp), %rdi
	xorl	%esi, %esi
	movq	%rdi, 8(%rsp)
	call	gettimeofday@PLT
	movq	40(%rsp), %r8
	subq	24(%rsp), %r8
	movabsq	$2361183241434822607, %r9
	movq	32(%rsp), %r15
	subq	16(%rsp), %r15
	leaq	.LC3(%rip), %rdi
	movq	%r13, %rsi
	leaq	64(%rsp), %r13
	movq	%r8, %rax
	sarq	$63, %r8
	imulq	%r9
	imull	$1000, %r15d, %r14d
	sarq	$7, %rdx
	subq	%r8, %rdx
	leal	(%r14,%rdx), %r15d
	movl	$-1431655765, %r14d
	call	print4f@PLT
	leaq	.LC4(%rip), %rsi
	movl	%r15d, %edx
	movl	$1, %edi
	xorl	%eax, %eax
	xorl	%r15d, %r15d
	call	__printf_chk@PLT
	movq	(%rsp), %rdi
	xorl	%esi, %esi
	call	gettimeofday@PLT
	.p2align 4,,10
	.p2align 3
.L8:
	pxor	%xmm13, %xmm13
	movl	%r15d, %r10d
	andl	$15, %r10d
	pxor	%xmm0, %xmm0
	movslq	%r10d, %r11
	pxor	%xmm2, %xmm2
	cvtsi2ss	%r10d, %xmm13
	sarl	%r10d
	movl	%r15d, %r8d
	movl	%r15d, %eax
	cvtsi2sd	%r10d, %xmm0
	andl	$3, %r8d
	pxor	%xmm14, %xmm14
	movl	%r15d, %esi
	cvtsi2ss	%r8d, %xmm2
	mull	%r14d
	pxor	%xmm15, %xmm15
	pxor	%xmm1, %xmm1
	shrl	%edx
	leal	(%rdx,%rdx,2), %ecx
	movq	%rbx, %rdx
	subl	%ecx, %esi
	movq	%r13, %rcx
	movslq	%esi, %rdi
	movq	%rbp, %rsi
	addsd	.LC2(%rip), %xmm0
	mulss	.LC1(%rip), %xmm13
	mulss	.LC1(%rip), %xmm2
	cvtsd2ss	%xmm0, %xmm1
	cvtss2sd	%xmm13, %xmm14
	subsd	.LC2(%rip), %xmm14
	movss	%xmm2, 48(%rsp,%rdi,4)
	movq	%r12, %rdi
	cvtsd2ss	%xmm14, %xmm15
	movss	%xmm1, 176(%rsp,%r11,4)
	movss	%xmm15, 112(%rsp,%r11,4)
	call	TransformModelToClip_SSE@PLT
	leal	1(%r15), %r9d
	pxor	%xmm3, %xmm3
	pxor	%xmm6, %xmm6
	addl	$2, %r15d
	movl	%r9d, %r10d
	pxor	%xmm8, %xmm8
	andl	$15, %r10d
	pxor	%xmm4, %xmm4
	cvtsi2ss	%r10d, %xmm3
	movslq	%r10d, %r11
	sarl	%r10d
	movl	%r9d, %eax
	cvtsi2sd	%r10d, %xmm6
	movl	%r9d, %esi
	andl	$3, %r9d
	pxor	%xmm5, %xmm5
	cvtsi2ss	%r9d, %xmm8
	mull	%r14d
	pxor	%xmm7, %xmm7
	shrl	%edx
	leal	(%rdx,%rdx,2), %ecx
	movq	%rbx, %rdx
	subl	%ecx, %esi
	movq	%r13, %rcx
	movslq	%esi, %rdi
	movq	%rbp, %rsi
	addsd	.LC2(%rip), %xmm6
	mulss	.LC1(%rip), %xmm3
	mulss	.LC1(%rip), %xmm8
	cvtsd2ss	%xmm6, %xmm7
	cvtss2sd	%xmm3, %xmm4
	subsd	.LC2(%rip), %xmm4
	movss	%xmm8, 48(%rsp,%rdi,4)
	movq	%r12, %rdi
	cvtsd2ss	%xmm4, %xmm5
	movss	%xmm7, 176(%rsp,%r11,4)
	movss	%xmm5, 112(%rsp,%r11,4)
	call	TransformModelToClip_SSE@PLT
	cmpl	$100000000, %r15d
	jne	.L8
	movq	8(%rsp), %rdi
	xorl	%esi, %esi
	call	gettimeofday@PLT
	movq	40(%rsp), %r8
	subq	24(%rsp), %r8
	movabsq	$2361183241434822607, %r9
	movq	32(%rsp), %r15
	subq	16(%rsp), %r15
	leaq	.LC5(%rip), %rdi
	movq	%r13, %rsi
	leaq	96(%rsp), %r13
	movq	%r8, %rax
	sarq	$63, %r8
	imulq	%r9
	imull	$1000, %r15d, %r14d
	sarq	$7, %rdx
	subq	%r8, %rdx
	leal	(%r14,%rdx), %r15d
	movl	$-1431655765, %r14d
	call	print4f@PLT
	leaq	.LC6(%rip), %rsi
	movl	%r15d, %edx
	movl	$1, %edi
	xorl	%eax, %eax
	xorl	%r15d, %r15d
	call	__printf_chk@PLT
	movq	(%rsp), %rdi
	xorl	%esi, %esi
	call	gettimeofday@PLT
	.p2align 4,,10
	.p2align 3
.L9:
	pxor	%xmm9, %xmm9
	movl	%r15d, %r10d
	andl	$15, %r10d
	pxor	%xmm12, %xmm12
	movslq	%r10d, %r11
	pxor	%xmm14, %xmm14
	cvtsi2ss	%r10d, %xmm9
	sarl	%r10d
	movl	%r15d, %r8d
	movl	%r15d, %eax
	cvtsi2sd	%r10d, %xmm12
	andl	$3, %r8d
	pxor	%xmm10, %xmm10
	movl	%r15d, %esi
	cvtsi2ss	%r8d, %xmm14
	mull	%r14d
	pxor	%xmm11, %xmm11
	pxor	%xmm13, %xmm13
	shrl	%edx
	leal	(%rdx,%rdx,2), %ecx
	movq	%rbx, %rdx
	subl	%ecx, %esi
	movq	%r13, %rcx
	movslq	%esi, %rdi
	movq	%rbp, %rsi
	addsd	.LC2(%rip), %xmm12
	mulss	.LC1(%rip), %xmm9
	mulss	.LC1(%rip), %xmm14
	cvtsd2ss	%xmm12, %xmm13
	cvtss2sd	%xmm9, %xmm10
	subsd	.LC2(%rip), %xmm10
	movss	%xmm14, 48(%rsp,%rdi,4)
	movq	%r12, %rdi
	cvtsd2ss	%xmm10, %xmm11
	movss	%xmm13, 176(%rsp,%r11,4)
	movss	%xmm11, 112(%rsp,%r11,4)
	call	TransformModelToClip_SSE2@PLT
	leal	1(%r15), %r9d
	pxor	%xmm15, %xmm15
	pxor	%xmm2, %xmm2
	addl	$2, %r15d
	movl	%r9d, %r10d
	pxor	%xmm4, %xmm4
	andl	$15, %r10d
	pxor	%xmm0, %xmm0
	cvtsi2ss	%r10d, %xmm15
	movslq	%r10d, %r11
	sarl	%r10d
	movl	%r9d, %eax
	cvtsi2sd	%r10d, %xmm2
	movl	%r9d, %esi
	andl	$3, %r9d
	pxor	%xmm1, %xmm1
	cvtsi2ss	%r9d, %xmm4
	mull	%r14d
	pxor	%xmm3, %xmm3
	shrl	%edx
	leal	(%rdx,%rdx,2), %ecx
	movq	%rbx, %rdx
	subl	%ecx, %esi
	movq	%r13, %rcx
	movslq	%esi, %rdi
	movq	%rbp, %rsi
	addsd	.LC2(%rip), %xmm2
	mulss	.LC1(%rip), %xmm15
	mulss	.LC1(%rip), %xmm4
	cvtsd2ss	%xmm2, %xmm3
	cvtss2sd	%xmm15, %xmm0
	subsd	.LC2(%rip), %xmm0
	movss	%xmm4, 48(%rsp,%rdi,4)
	movq	%r12, %rdi
	cvtsd2ss	%xmm0, %xmm1
	movss	%xmm3, 176(%rsp,%r11,4)
	movss	%xmm1, 112(%rsp,%r11,4)
	call	TransformModelToClip_SSE2@PLT
	cmpl	$100000000, %r15d
	jne	.L9
	movq	8(%rsp), %rdi
	xorl	%esi, %esi
	movabsq	$2361183241434822607, %r15
	call	gettimeofday@PLT
	movq	40(%rsp), %r12
	subq	24(%rsp), %r12
	leaq	.LC7(%rip), %rdi
	movq	32(%rsp), %rbx
	subq	16(%rsp), %rbx
	movq	%r13, %rsi
	movq	%r12, %rax
	sarq	$63, %r12
	imulq	%r15
	imull	$1000, %ebx, %ebp
	sarq	$7, %rdx
	subq	%r12, %rdx
	leal	0(%rbp,%rdx), %r14d
	call	print4f@PLT
	leaq	.LC8(%rip), %rsi
	xorl	%eax, %eax
	movl	%r14d, %edx
	movl	$1, %edi
	call	__printf_chk@PLT
	movq	248(%rsp), %rbx
	xorq	%fs:40, %rbx
	movl	$1, %eax
	jne	.L18
	addq	$264, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L18:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE65:
	.size	main, .-main
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC1:
	.long	1051372203
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC2:
	.long	2696277389
	.long	1051772663
	.ident	"GCC: (Ubuntu 7.3.0-27ubuntu1~18.04) 7.3.0"
	.section	.note.GNU-stack,"",@progbits
