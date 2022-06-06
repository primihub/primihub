
        .file   "mrmuldv.s"
        .text
        .globl  muldiv
muldiv:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %ebx


        movl    8(%ebp),%eax  
        mull    12(%ebp)      
        addl    16(%ebp),%eax 
        adcl    $0,%edx       

        divl    20(%ebp)       
        movl    24(%ebp),%ebx 
        movl    %edx,(%ebx)
    
        popl    %ebx
        popl    %ebp
        ret

        .globl  muldvm
muldvm:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %ebx

        movl    8(%ebp),%edx  
        movl    12(%ebp),%eax 
        divl    16(%ebp)      

        movl    20(%ebp),%ebx 
        movl    %edx,(%ebx)   

        popl    %ebx
        popl    %ebp
        ret

        .globl  muldvd
muldvd:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %ebx

        movl    8(%ebp),%eax  
        mull    12(%ebp)      
        addl    16(%ebp),%eax 
        adcl    $0,%edx       
        movl    20(%ebp),%ebx 
        movl    %eax,(%ebx)   
        movl    %edx,%eax     

        popl    %ebx
        popl    %ebp
        ret

        .globl  muldvd2
muldvd2:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %ebx
        pushl   %esi

        movl    8(%ebp),%eax  
        mull    12(%ebp)
        movl    16(%ebp),%ebx
        addl    (%ebx),%eax
        adcl    $0,%edx       
        movl    20(%ebp),%esi
        addl    (%esi),%eax
        adcl    $0,%edx

        movl    %eax,(%esi)   
        movl    %edx,(%ebx)     

        popl    %esi
        popl    %ebx
        popl    %ebp
        ret

