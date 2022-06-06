/*
 *  Turbo C compiler V1.5+, Turbo/Borland C++. Microsoft C/C++ 
 *  Uses inline assembly feature
 *  Generates code identical to above version, and
 *  can be used instead. 
 */

#define ASM asm

/* or perhaps #define ASM _asm  */

unsigned int muldiv(a,b,c,m,rp)
unsigned int a,b,c,m,*rp;
{
    ASM mov  ax,a             ;/* get a                     */
    ASM mul  WORD PTR b       ;/* multiply by b             */
    ASM add  ax,c             ;/* add c to low word         */
    ASM adc  dx,0h            ;/* add carry to high word    */
    ASM div  WORD PTR m       ;/* divide by m               */
    ASM mov  bx,rp            ;/* get address for remainder */
    ASM mov  [bx],dx          ;/* store remainder           */
}
/*    Replace last two ASM lines when using large data memory models */
/*    ASM les  bx, DWORD PTR rp          ; get address for remainder */
/*    ASM mov  WORD PTR es:[bx],dx       ; store remainder           */

unsigned int muldvm(a,c,m,rp)
unsigned int a,c,m,*rp;
{
    ASM mov dx,a              ;/* get a                     */
    ASM mov ax,c              ;/* add in c to low word      */
    ASM div WORD PTR m        ;/* divide by m               */
    ASM mov bx,rp             ;/* get address for remainder */
    ASM mov [bx],dx           ;/* store remainder           */
}
/*    Replace last two ASM lines when using large data memory models */
/*    ASM les  bx, DWORD PTR rp          ; get address for remainder */
/*    ASM mov  WORD PTR es:[bx],dx       ; store remainder           */

unsigned int muldvd(a,b,c,rp)
unsigned int a,b,c,*rp;
{
    ASM mov  ax,a             ;/* get a                     */
    ASM mul  WORD PTR b       ;/* multiply by b             */
    ASM add  ax,c             ;/* add c to low word         */
    ASM adc  dx,0h            ;/* add carry to high word    */
    ASM mov  bx,rp            ;/* get address for remainder */
    ASM mov  [bx],ax          ;/* store remainder           */
    ASM mov  ax,dx
}
/*    Replace second and third last lines if using large data memory models */
/*    ASM les  bx, DWORD PTR rp          ; get address for remainder */
/*    ASM mov  WORD PTR es:[bx],ax       ; store remainder           */

void muldvd2(a,b,c,rp)
unsigned int a,b,*c,*rp;
{
    ASM mov  ax,a             ;/* get a                     */
    ASM mul  WORD PTR b       ;/* multiply by b             */
    ASM mov  bx,c
    ASM add  ax,[bx]
    ASM adc  dx,0h            ;/* add carry to high word    */
    ASM mov  si,rp
    ASM add  ax,[si]
    ASM adc  dx,0h
    ASM mov  [si],ax
    ASM mov  [bx],dx
}

/* for large memory model ....
    ASM mov  ax,a            
    ASM mul  WORD PTR b     
    ASM les  bx, DWORD PTR c
    ASM add  ax, WORD PTR es:[bx]
    ASM adc  dx,0h          
    ASM les  si,DWORD PTR rp
    ASM add  ax,WORD PTR es:[si]
    ASM adc  dx,0h
    ASM mov  WORD PTR es:[si],ax
    ASM les  bx,DWORD PTR c
    ASM mov  WORD PTR es:[bx],dx
*/
