    .text
    .align 32                           # 1. function entry alignment
    .globl MatrixMultiply4x4_ASM            #    (for a faster call)
    .type MatrixMultiply4x4_ASM, @function
MatrixMultiply4x4_ASM:
    movaps   (%rdi), %xmm0
    movaps 16(%rdi), %xmm1
    movaps 32(%rdi), %xmm2
    movaps 48(%rdi), %xmm3
    movq $48, %rcx                      # 2. loop reversal
1:                                      #    (for simpler exit condition)
    movss (%rsi, %rcx), %xmm4           # 3. extended address operands
    shufps $0, %xmm4, %xmm4             #    (faster than pointer calculation)
    mulps %xmm0, %xmm4
    movaps %xmm4, %xmm5
    movss 4(%rsi, %rcx), %xmm4
    shufps $0, %xmm4, %xmm4
    mulps %xmm1, %xmm4
    addps %xmm4, %xmm5
    movss 8(%rsi, %rcx), %xmm4
    shufps $0, %xmm4, %xmm4
    mulps %xmm2, %xmm4
    addps %xmm4, %xmm5
    movss 12(%rsi, %rcx), %xmm4
    shufps $0, %xmm4, %xmm4
    mulps %xmm3, %xmm4
    addps %xmm4, %xmm5
    movaps %xmm5, (%rdx, %rcx)
    subq $16, %rcx                      # one 'sub' (vs 'add' & 'cmp')
    jge 1b                              # SF=OF, idiom: jump if positive
    ret
