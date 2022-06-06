/*
 *      MIRACL RATIONAL CALCULATOR - IBM-PC VERSION
 *      Compiles with Turbo C V1.0+, Microsoft C V3.0+ or Zortech C++ V2.0
 *  **  Change display mode for 40/80 column operation (e.g. 'mode 40' to DOS)
 *  **  Change colour/B&W below
 */

#include <stdio.h>     /*** IBM-PC specific section ***/
#include <string.h>
#include <dos.h>        
#include "miracl.h"

#define ESC 27
#define SPACE 32
#define BELL 7

#define DVERT 186
#define DHORZ 205
#define DTLHC 201
#define DTRHC 187
#define DBLHC 200
#define DBRHC 188
#define VERT 179        
#define HORZ 196
#define TLHC 218
#define TRHC 191
#define BLHC 192
#define BRHC 217
#define LSIDE 199
#define RSIDE 182

/* Globals */
/* set colours          B/W       Colours (suggested) */

static int ORDINARY=0x17; /*   0x07        0x17    blue-white        */
static int INVER   =0x40; /*   0x70        0x40    red-black         */
static int BOLD    =0x70; /*   0x0F        0x70    white-black       */
static int BLINKING=0xF4; /*   0x87        0xF4    white-red (blink) */
static int HELPCOL =0x02; /*   0x07        0x02    black-green       */
static int PRESSED =0x4F; /*   0x0F        0x4F    red-white (bold)  */
static int STATCOL =0x74; /*   0x07        0x74    white-red         */
static int BGROUND =0x07; /*   0x07        0x07    black-white       */

static int dmode;

#if MIRACL==16
                /*** Constants 16-bit values ***/

static char cpi[] = "636254619847503442989626/202526135627569822173415";
static char clg2[]= "505741081295988878347013/729630149959545241557174";
static char clg10[]="979037493951642967431763/425190581199586827388880";

#else
                /*** Constants 32-bit values ***/

static char cpi[] = "67165660610256098973103849091/21379493784309731162770152371";
static char clg2[]= "16574595208316928044019202463/23912086311780807469140302041";
static char clg10[]="2379577069267877188193568690/1033437190446551110271192309";

#endif

static char ceps[]="1/100000000000000000000000000000000000";

                /*** Device independent data ***/

static char *settings[4][4]= 
{"   ","HYP","","",
 "Be ","B2 ","B10","",
 "RAD","DEG","GRA","",
 "DEC","HEX","OCT","BIN"};

static int nops[]= {1,2,1,3};         /* number of options   */
static int opp[] = {0,1,1,2,2,3,3,2}; /* operator precedence */

static char *keys[6][7]= 
{"SIN","COS","TAN","EXP","F-D","CLR","OFF",
 "ASN","ACS","ATN","LOG","SET","MOD","DEL",
 " 7 "," 8 "," 9 ","Y^X","XûY"," ö "," RM",    /******/
 " 4 "," 5 "," 6 "," Xý","ýûX"," x "," M+",
 " 1 "," 2 "," 3 ","1/X"," ã "," - ","+/-",
 " 0 "," / "," . "," ( "," ) "," + "," = "};

static int qkeys[6][7]=  
{'s','k','t','e','f','c','o',
 'S','K','T','l','g','%','<',
 '7','8','9','y','x','\\','r',
 '4','5','6','q','v','*','m',
 '1','2','3','i','p','-',';',
 '0','/','.','(',')','+','='};

static char *htext[]=  
{"Arrow keys find, space bar activates OR",
 "use numeric keys  (with  A-F  for hex),",
 "brackets,  and mnemonic keys as below: ",
 " FUNCTION      KEY   FUNCTION       KEY",
 "   SIN          s      ASN           S ",
 "   COS          k      ACS           K ",
 "   TAN          t      ATN           T ",
 "   EXP(e/2/10)  e      LOG(e/2/10)   l ",
 "   Y^X     ^ or y      XûY           x ",   /******/
 "    Xý          q      ýûX           v ",
 "   1/X          i       ã            p ",
 "   F-D          f      SET           g ",
 "   MOD          %      CLR           c ",
 "    ö      d or \\       x       b or * ",
 "    -           -       +       , or + ",
 "   OFF   ESC or o      DEL    DEL or < ",
 "    RM          r       M+           m ",
 "   +/-          ;       =     RET or = ",
 "Note: / means 'over'  as in fractions  ",
 "F-D converts  fraction  <-->  decimal  ",
 "SET changes settings - use arrow keys  ",
 "Alternative arrow keys -   u  h  j  n  ",
 "This program is Public Domain -        ",
 "Certivox, Ireland mscott@indigo.ie     "};     /******/

static char display[]=   "   MIRACL RATIONAL CALCULATOR V3.2   ";
static char  status[]=   "                                     ";
static char oldstat[]=   "                                     ";

static miracl *mip;

static int erptr=0;
static int mmptr=6;
static int exptr=10;
static int typtr=16;
static int stptr[]={22,26,30,34};

static int dbeg=2;
static int dlen=37;
static int top=7;
static int width=80;      /* screen width can be 40 or 80 columns */
static char mybuff[40];

static flash x,y[8],m,t;
static flash radeg,loge2,loge10,eps;
static int ipt,op[8],prop[8],sp,brkt,lgbase;
static BOOL flag,newx,result,hyp,degrees,delim;
static int option[]={0,0,0,0};

/* Device specific code - IBM-PC versions */

static void curser(int x,int y)
{ /* position cursor at x,y */
    union REGS regs;
    regs.h.ah=2;
    regs.h.dh=y-1;
    regs.h.dl=x-1;
    regs.h.bh=0;              /* Video page 0 */
    int86(0x10,&regs,&regs);  /* Use Dos Interrupt 10h */
}   

static void screen(void)
{ /* initialise screen */
    union REGS regs;
    regs.h.ah=0x0F;  /* get screen mode */
    regs.h.al=0;
    int86(0x10,&regs,&regs);
    dmode=regs.h.al;
    if ((dmode&2)!=0) width=80;
    else              width=40;
    regs.h.ah=6;     /* clear screen */
    regs.h.al=0;
    regs.h.cl=0;
    regs.h.ch=0;
    regs.h.dl=width-1;
    regs.h.dh=24;
    regs.h.bh=BGROUND;
    regs.h.bl=0;
    int86(0x10,&regs,&regs);
}

static void restore(void)
{  /* restore situation */
    union REGS inregs,outregs;
    inregs.h.ah=0;       /* reset initial display mode */
    inregs.h.al=dmode;
    int86(0x10,&inregs,&outregs);
}

static void apchar(int attr,int x,int y,char ch)
{ /* output character with attribute at x,y */
    union REGS regs;
    curser(x,y);
    regs.h.ah=9;              /* output a character */
    regs.h.al=ch;
    regs.h.bh=0;
    regs.h.bl=attr;
    regs.h.cl=1;
    regs.h.ch=0;
    int86(0x10,&regs,&regs);
}

static void aprint(int attr,int x,int y,char *text)
{ /* attribute print */
    while (*text!='\0')
    {
        apchar(attr,x++,y,*text);
        text++;
    }
}

static void lclr(int x,int y)
{ /* clear from x,y to end of line */
    aprint(BGROUND,x,y,"                                       ");
}

static void cset(int k)
{ /* select special character set */
    return;
}

static int gethit(void)
{ /* get single keystroke */
    int ch;
    ch=getch();
    if (ch!=0) return ch;
    ch=getch();
    if (ch==72) return 'u';  /* transform some useful extended codes */
    if (ch==75) return 'h';
    if (ch==77) return 'j';
    if (ch==80) return 'n';
    if (ch==83) return 127;
    return 0;
}

/*** Device independent code ***/

static int arrow(int c)
{ /* check for arrow key             *
   * returns 1 for up, 2 for down,   *
   * 3 for right, 4 for left, else 0 */
    if (c=='u') return 1;
    if (c=='n') return 2;
    if (c=='j') return 3;
    if (c=='h') return 4;
    return 0;
}

static void instat(int ptr,char *strg)
{ /* insert a status setting into status line */
    strncpy(&status[ptr],strg,strlen(strg));
}

static void getstat(void)
{ /* set status line */
    int i;
    if (mip->ERNUM) instat(erptr,"ERROR");
    else       instat(erptr,"     ");
    if (mip->EXACT) instat(exptr,"EXACT");
    else       instat(exptr,"     ");
    if (size(m)!=0) instat(mmptr,"MEM");
    else            instat(mmptr,"   ");
    if (mip->RPOINT) instat(typtr,"POINT");
    else        instat(typtr,"FRACT");
    for (i=0;i<4;i++)
         instat(stptr[i],settings[i][option[i]]);
}

static void setopts(void)
{ /* set options */
    if (option[0]==0) hyp=FALSE;
    else              hyp=TRUE;
    lgbase=0;
    if (option[1]==1) lgbase=2;
    if (option[1]==2) lgbase=10;
    if (option[2]==0) degrees=FALSE;
    else              degrees=TRUE;
    mip->IOBASE=10;
    if (option[3]==1) mip->IOBASE=16;
    if (option[3]==2) mip->IOBASE=8;
    if (option[3]==3) mip->IOBASE=2;
} 

static void show(BOOL force)
{ /* output display */
    if (force || strcmp(oldstat,status)!=0)
    {
        if (mip->ERNUM) aprint(BLINKING,2,2,status);
        else  aprint(STATCOL,2,2,status);
        strcpy(oldstat,status);
    }
    aprint(BOLD,dbeg,3,display);
}

static void clr(void)
{ /* clear input buffer */
    mybuff[0]='0';
    mybuff[1]='\0';
    ipt=0;
    delim=FALSE;
}

static void just(char *buff)
{ /* justify buff into display (if possible) *
   * and update status                       */
    int i,mp,df;
    BOOL dot;
    dot=FALSE;
    mp=0;
    while (mp<=dlen && buff[mp]!='\0')
    {
        if (buff[mp]=='.') dot=TRUE;
        if (mp==dlen && !dot) mip->ERNUM=(-1);
        mp++;
    }
    if (mp>dlen) mp=dlen;
    if (!mip->ERNUM)
    {
        df=dlen-mp;
        for (i=0;i<df;i++) display[i]=' ';
        for (i=0;i<mp;i++) display[df+i]=buff[i];
    }
    display[dlen]='\0';
}

static void drawit(void)
{ /* Draw calculator */
    int i,j;
    char line[40],tline[40],bline[40];
    cset(1);
    line[0]=DTLHC;
    for (i=1;i<38;i++) line[i]=DHORZ;
    line[38]=DTRHC;
    line[39]='\0';
    aprint(ORDINARY,1,1,line);
    for (i=1;i<22;i++) apchar(ORDINARY,1,1+i,DVERT); 
    line[0]=DBLHC;
    line[38]=DBRHC;
    aprint(ORDINARY,1,23,line);
    for (i=1;i<22;i++) apchar(ORDINARY,39,1+i,DVERT);
    line[0]=LSIDE;  /* draw in the bar */
    for (i=1;i<38;i++) line[i]=HORZ;
    line[38]=RSIDE;
    aprint(ORDINARY,1,4,line);
    line[0]=tline[0]=bline[0]=SPACE;
    line[36]=tline[36]=bline[36]=SPACE;
    line[37]=tline[37]=bline[37]='\0';
    for (i=1;i<36;i++)
    {
        switch (i%5)
        {
        case 1 : tline[i]=TLHC;
                 bline[i]=BLHC;
                 line[i]=VERT;
                 break;
        default: tline[i]=HORZ;
                 bline[i]=HORZ;
                 line[i]=SPACE;
                 break;
        case 0 : tline[i]=TRHC;
                 bline[i]=BRHC;
                 line[i]=VERT;
                 break;
        }
    } 
    for (j=0;j<6;j++)
    {
        aprint(ORDINARY,2,5+3*j,tline);
        aprint(ORDINARY,2,6+3*j,line);
        aprint(ORDINARY,2,7+3*j,bline);
    }
    cset(0);
    for (j=0;j<6;j++)
        for (i=0;i<7;i++)
            aprint(ORDINARY,4+5*i,6+3*j,keys[j][i]);
    aprint(HELPCOL,2,24,"Type 'H' for help on/off, 'O' to exit");
    cotstr(x,mip->IOBUFF);
    just((char *)mip->IOBUFF);
    getstat();
    show(TRUE);
}

static BOOL next(int ch)
{ /* get next digit - returns FALSE if there is a problem  */
    int cv;
    result=FALSE;
    if (ipt>=dlen) return FALSE;
    if (ch=='/' || ch=='.')
    {
        if (delim || (ch=='/' && ipt==0)) return FALSE;
        delim=TRUE;
    }
    else
    {
        if (ch>='A' && ch<='F') cv=10+(ch-'A');
        else                    cv=ch-'0';
        if (mip->IOBASE<=cv) return FALSE;
    }
    if (ipt==0 && ch=='0') clr();
    else
    {
        mybuff[ipt++]=ch;
        mybuff[ipt]='\0';
    }
    just(mybuff);
    cinstr(x,mybuff);
    newx=TRUE;
    return TRUE;
}

static void swap(void)
{ /* swap x with top of stack */
    flash t;
    t=x;
    x=y[sp];
    y[sp]=t;
}

static int prec(int no)
{ /* return new operator precedence */
    return 4*brkt+opp[no];
}

static void equals(int no)
{ /* perform binary operation */
    BOOL pop;
    newx=FALSE;
    pop=TRUE;
    while (pop)
    {
        if (prec(no)>prop[sp])
        { /* if higher precedence, keep this one pending */
            if (sp==top)
            {
                mip->ERNUM=(-1);
                result=FALSE;
                newx=TRUE;
            }
            else sp++;
            return;
        }
        newx=TRUE;
        if (flag && op[sp]!=3 && op[sp]!=0) swap();
        switch (op[sp])
        {
        case 7: fdiv(x,y[sp],t);
                ftrunc(t,t,t);
                fmul(t,y[sp],t);
                fsub(x,t,x);
                break;
        case 6: fpowf(x,y[sp],x);
                break;
        case 5: frecip(y[sp],t);
                fpowf(x,t,x);
                break;
        case 4: fdiv(x,y[sp],x);
                break;
        case 3: fmul(x,y[sp],x);
                break;
        case 2: fsub(x,y[sp],x);
                break;
        case 1: fadd(x,y[sp],x);
                break;
        case 0: break;
        }
        if (sp>0 && (prec(no)<=prop[sp-1])) sp--;
        else pop=FALSE;
    }
}

static void chekit(int no)
{ /* deal with operator */
    if (flag && newx) equals(no);
    else newx=FALSE;
    flag=ON;
    copy(x,y[sp]);
    op[sp]=no;
    prop[sp]=prec(no);
}

static void clrall(void)
{ /* clear all */
    mip->ERNUM=0;
    zero(x);
    zero(y[0]);
    zero(m);
    sp=0;
    op[sp]=0;
    prop[sp]=0;
    brkt=0;
    mip->RPOINT=ON;
    mip->EXACT=TRUE;
}

static BOOL act(int p,int q)
{ /* act on selected key */
    int k,n,c;
    aprint(PRESSED,4+5*p,6+3*q,keys[q][p]);
    switch(p+7*q)
    {
    case 0:  if (degrees) fmul(x,radeg,x);
             if (hyp) fsinh(x,x);
             else     fsin(x,x);
             newx=TRUE;
             break;
    case 1:  if (degrees) fmul(x,radeg,x);
             if (hyp) fcosh(x,x);
             else     fcos(x,x);
             newx=TRUE;
             break;
    case 2:  if (degrees) fmul(x,radeg,x);
             if (hyp) ftanh(x,x);
             else     ftan(x,x);
             newx=TRUE;
             break;
    case 3:  if (lgbase>0)
             {
                 n=size(x);
                 if (abs(n)<MR_TOOBIG)
                 {
                     convert(lgbase,x);
                     if (n<0) frecip(x,x);
                     fpower(x,abs(n),x);
                     newx=TRUE;
                     break;
                 }
                 if (lgbase==2)  fmul(x,loge2,x);
                 if (lgbase==10) fmul(x,loge10,x);
             }
             fexp(x,x);
             newx=TRUE;
             break;
    case 4:  mip->RPOINT=!mip->RPOINT;
             newx=TRUE;
             break;
    case 5:  clrall();
             newx=TRUE;
             break;
    case 6:  return TRUE;
    case 7:  if (hyp) fasinh(x,x);
             else     fasin(x,x);
             if (degrees) fdiv(x,radeg,x);
             newx=TRUE;
             break;
    case 8:  if (hyp) facosh(x,x);
             else     facos(x,x);
             if (degrees) fdiv(x,radeg,x);
             newx=TRUE;
             break;
    case 9:  if (hyp) fatanh(x,x);
             else     fatan(x,x);
             if (degrees) fdiv(x,radeg,x);
             newx=TRUE;
             break;
    case 10: flog(x,x);
             if (lgbase==2)  fdiv(x,loge2,x);
             if (lgbase==10) fdiv(x,loge10,x);
             newx=TRUE;
             break;
    case 11: newx=TRUE;
             k=3;
             forever
             {
                 aprint(INVER,2+stptr[k],2,settings[k][option[k]]);
                 curser(2+stptr[k],2);
                 c=arrow(gethit());
                 if (c==1)
                 {
                     if (option[k]==nops[k]) option[k]=0;
                     else option[k]+=1;
                     continue;
                 }
                 aprint(STATCOL,2+stptr[k],2,settings[k][option[k]]);
                 if (c==0 || c==2) break;
                 if (c==4 && k>0) k--;
                 if (c==3 && k<3) k++;
             }
             setopts();
             break;
    case 12: chekit(7);
             break;
    case 13: result=FALSE;
             if (ipt==0) break;
             ipt--;
             mybuff[ipt]='\0';
             if (ipt==0) clr();
             just(mybuff);
             cinstr(x,mybuff);
             newx=TRUE;
             break;
    case 14: if (!next('7')) putchar(BELL);
             break;
    case 15: if (!next('8')) putchar(BELL);
             break;
    case 16: if (!next('9')) putchar(BELL);
             break;
    case 17: chekit(6);
             break;
    case 18: chekit(5);
             break;
    case 19: chekit(4);
             break;
    case 20: copy(m,x);
             newx=TRUE;
             break;
    case 21: if (!next('4')) putchar(BELL);
             break;
    case 22: if (!next('5')) putchar(BELL);
             break;
    case 23: if (!next('6')) putchar(BELL);
             break;
    case 24: fmul(x,x,x);
             newx=TRUE;
             break;
    case 25: froot(x,2,x);
             newx=TRUE;
             break;
    case 26: chekit(3);
             break;
    case 27: brkt=0;
             chekit(0);
             flag=OFF;
             fadd(m,x,m);
             newx=TRUE;
             break;
    case 28: if (!next('1')) putchar(BELL);
             break;
    case 29: if (!next('2')) putchar(BELL);
             break;
    case 30: if (!next('3')) putchar(BELL);
             break;
    case 31: frecip(x,x);
             newx=TRUE;
             break;
    case 32: fpi(x);
             newx=TRUE;
             break;
    case 33: chekit(2);
             break;
    case 34: negify(x,x);
             newx=TRUE;
             break;
    case 35: if (!next('0')) putchar(BELL);
             break;
    case 36: if (!next('/')) putchar(BELL);
             break;
    case 37: if (!next('.')) putchar(BELL);
             break;
    case 38: if (ipt>0)
             {
                 putchar(BELL);
                 result=FALSE;
             }
             else
             {
                 zero(x);
                 brkt+=1;
                 newx=TRUE;
             }
             break;
    case 39: if (brkt>0)
             {
                 chekit(0);
                 brkt-=1;
             }
             else
             {
                 putchar(BELL);
                 result=FALSE;
             }
             break;
    case 40: chekit(1);
             break;
    case 41: brkt=0;
             equals(0);
             flag=OFF;
             break;
    }
    return FALSE;
}

int main()
{ /*  MIRACL rational calculator */
    int i,j,k,p,q,c,hpos;
    BOOL over,help;
    screen();
#if MIRACL==16
    mip=mirsys(10,0);      /*** 16-bit computer ***/
#else
    mip=mirsys(6,0);       /*** 32-bit computer ***/
#endif
    mip->ERCON=TRUE;
    x=mirvar(0);
    for (i=0;i<=top;i++) y[i]=mirvar(0);
    m=mirvar(0);
    t=mirvar(0);
    radeg=mirvar(0);
    loge2=mirvar(0);
    loge10=mirvar(0);
    eps=mirvar(0);
    mip->pi=mirvar(0);
    cinstr(mip->pi,cpi);            /* read in constants */
    fpmul(mip->pi,1,180,radeg);
    cinstr(loge2,clg2);
    cinstr(loge10,clg10);
    cinstr(eps,ceps);
    help=OFF;
    show(TRUE);
    p=6;
    q=0;
    flag=OFF;
    newx=OFF;
    over=FALSE;


    setopts();
    clrall();
    drawit();
    while (!over)
    { /* main loop */
        if (mip->ERNUM)
        {
            aprint(ORDINARY,4+5*p,6+3*q,keys[q][p]);
            p=5,q=0;
        }
        if (width==80 || !help)
        {
            aprint(INVER,4+5*p,6+3*q,keys[q][p]);
            curser(1,24);
            c=gethit();
            aprint(ORDINARY,4+5*p,6+3*q,keys[q][p]);
        }
        else while ((c=gethit())!='H') ;
        result=TRUE;
        if ((k=arrow(c))!=0)
        { /* arrow key hit */
            if (k==1 && q>0) q--;
            if (k==2 && q<5) q++;
            if (k==3 && p<6) p++;
            if (k==4 && p>0) p--;
            continue;
        }
        if (c=='H')
        { /* switch help on/off */
            help=!help;
            for (i=1;i<=24;i++)
            {
                if (width==80) hpos=41;
                else           hpos=1;
                if (help) aprint(HELPCOL,hpos,i,htext[i-1]);
                else lclr(hpos,i);
            }
            if (width==40 && !help) drawit();
            continue;
        }            
        if (c>='A' && c<='F')
        { /* hex only */
            if (!next(c)) putchar(BELL);
            else show(FALSE);
            continue;
        }
        for (j=0;j<6;j++)
            for (i=0;i<7;i++)
                if (c==qkeys[j][i]) p=i,q=j,c=' ';
        if (c==8 || c==127) p=6,q=1,c=' ';       /* aliases */
        if (c==',' || c=='a') p=5,q=5,c=' ';
        if (c=='O' || c==ESC) p=6,q=0,c=' ';
        if (c==13)  p=6,q=5,c=' ';
        if (c=='[' || c=='{') p=3,q=5,c=' ';
        if (c==']' || c=='}') p=4,q=5,c=' ';
        if (c=='d') p=5,q=2,c=' ';
        if (c=='b') p=5,q=3,c=' ';
        if (c=='^') p=3,q=2,c=' ';
        if (c==' ') over=act(p,q);
        else        continue;
        absol(x,t);
        if (fcomp(t,eps)<0) zero(x);
        if (result)
        { /* output result to display */
            cotstr(x,mip->IOBUFF);
            just((char *)mip->IOBUFF);
            if (mip->ERNUM<0)
            { /* convert to radix and try again */
                mip->ERNUM=0;
                mip->RPOINT=ON;
                cotstr(x,mip->IOBUFF);
                putchar(BELL);
                just((char *)mip->IOBUFF);
            }
            clr();
        }
        if (newx)
        { /* update display */
            getstat();
            show(FALSE);
        }
    }
    curser(1,24);
    restore();
    return 0;
}

