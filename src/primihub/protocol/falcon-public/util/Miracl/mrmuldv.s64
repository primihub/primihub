
        .file   "mrmuldv.s"
.text
	.globl  muldiv
muldiv:

        pushq   %rbx
        movq    %rdi,%rax
        movq    %rdx,%rbx
        mulq    %rsi
        addq    %rbx,%rax
        adcq    $0,%rdx

        divq    %rcx
        movq    %r8,%rbx
        movq    %rdx,(%rbx)
        popq    %rbx

        ret

        .globl muldvm
muldvm:
   
        pushq   %rbx
        movq %rdx,%rbx
        movq %rdi,%rdx
        movq %rsi,%rax
        divq %rbx

        movq %rcx,%rbx
        movq %rdx,(%rbx)
        popq    %rbx

        ret

        .globl muldvd
muldvd:
          
        pushq   %rbx
        movq %rdi,%rax
        movq %rdx,%rbx
        mulq %rsi
        addq %rbx,%rax
        adcq $0,%rdx

        movq %rcx,%rbx
        movq %rax,(%rbx)
        movq %rdx,%rax
        popq    %rbx

        ret

        .globl muldvd2
muldvd2:

        pushq   %rbx
        movq %rdi,%rax
        movq %rdx,%rbx
        mulq %rsi
        addq (%rbx),%rax
        adcq $0,%rdx
        addq (%rcx),%rax
        adcq $0,%rdx

        movq %rax,(%rcx)
        movq %rdx,(%rbx)
        popq    %rbx

        ret

