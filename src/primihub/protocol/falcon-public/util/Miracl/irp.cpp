//
// Program to generate code for reduction modulo an irreducible trinomial or pentanomial
// over the field GF(2^m), m a prime
//
// cl /O2 /GX irp.cpp
//
// code is generated and can be cut-and-pasted into the function 
// reduce2(.) in the MIRACL module mrgf2m.c
//
// To find a suitable irreducible polynomial, see the program findbase.cpp
//


#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    int i,j,W,M,A,B,C,k1,k2,k3,k4,rs1,rs2,rs3,rs4,ls1,ls2,ls3,ls4;
    int ind[64],shift[64][8],xxor,nxor,nsh;

	argc--; argv++;

    if (argc<3 || argc>5)
    {
        cout << "incorrect usage" << endl;
        cout << "Program generates reduction code for given irreducible polynomial" << endl;
        cout << "of the form x^M+x^A+1 or x^M+x^A+x^B+x^C+1 for odd M" << endl;
        cout << "for a computer with wordlength W" << endl;
        cout << "irp <W> <M> <A>" << endl;
        cout << "OR" << endl;
        cout << "irp <W> <M> <A> <B> <C>" << endl;
        cout << "The code that is generated can be copied into the" << endl;
        cout << "function reduce2 in the module mrgf2m.c" << endl;
        return 0;
    }

    A=B=C=0;
    W=atoi(argv[0]);
    M=atoi(argv[1]);
	A=atoi(argv[2]);
    if (W<0 || W%8!=0 || A<1 || M-A < W) exit(0);

    if (argc==5)
    {
        B=atoi(argv[3]);
        C=atoi(argv[4]);
        if (B<1 || C<1 || B>A || C>B) exit(0);
    }

    k1=1+M/W;
    
    rs1=M%W;
    ls1=W-rs1;

    k2=1+(M-A)/W; 

    rs2=(M-A)%W;
    ls2=W-rs2;

    if (B)
    {
        k3=1+(M-B)/W;
        rs3=(M-B)%W;
        ls3=W-rs3;

        k4=1+(M-C)/W;
        rs4=(M-C)%W;
        ls4=W-rs4;
    }

    for (i=0;i<64;i++) ind[i]=0;
  
    if (rs1==0) 
    {
        shift[k1-1][ind[k1-1]]=0;
        ind[k1-1]++;
    }
    else
    {
        shift[k1-1][ind[k1-1]]=rs1;
        ind[k1-1]++;
        shift[k1][ind[k1]]=-ls1;
        ind[k1]++;
    }
    
    if (rs2==0) 
    {
        shift[k2-1][ind[k2-1]]=0;
        ind[k2-1]++;
    }
    else
    {
        shift[k2-1][ind[k2-1]]=rs2;
        ind[k2-1]++;
        shift[k2][ind[k2]]=-ls2;
        ind[k2]++;
    }

    if (B)
    {
        if (rs3==0) 
        {
            shift[k3-1][ind[k3-1]]=0;
            ind[k3-1]++;
        }
        else
        {
            shift[k3-1][ind[k3-1]]=rs3;
            ind[k3-1]++;
            shift[k3][ind[k3]]=-ls3;
            ind[k3]++;
        }
        if (rs4==0) 
        {
            shift[k4-1][ind[k4-1]]=0;
            ind[k4-1]++;
        }
        else
        {
            shift[k4-1][ind[k4-1]]=rs4;
            ind[k4-1]++;
            shift[k4][ind[k4]]=-ls4;
            ind[k4]++;
        }
    }

    if (B) printf("    if (M==%d && A==%d && B==%d && C==%d)\n",M,A,B,C);
    else   printf("    if (M==%d && A==%d)\n",M,A);
  
    printf("    {\n        for (i=xl-1;i>=%d;i--)\n        {\n",k1);
    printf("            w=gx[i]; gx[i]=0;\n");

    nxor=0; nsh=0;
    for (i=0;i<64;i++)
    {
        if (ind[i]==0) continue;
        nxor++;
        printf("            gx[i-%d]^=",i);
        xxor=0;
        for (j=0;j<ind[i];j++)
        {
            if (xxor) printf("^");
            if (shift[i][j]==0) {printf("w"); xxor++;}
            if (shift[i][j]>0) {printf("(w>>%d)",shift[i][j]); xxor++; nsh++; }
            if (shift[i][j]<0) {printf("(w<<%d)",-shift[i][j]); xxor++; nsh++;}
        }

        nxor+=(xxor-1);
        printf(";\n");  
    }
    printf("        }   /* XORs= %d shifts= %d */\n",nxor,nsh);
    printf("        top=gx[%d]>>%d; ",k1-1,rs1);
    printf("gx[0]^=top; ");
    printf("top<<=%d;\n",rs1);

    for (i=0;i<64;i++) ind[i]=0;

    if (rs2==0) 
    {
        shift[k1-k2][ind[k1-k2]]=0;
        ind[k1-k2]++;
    }
    else
    {
        shift[k1-k2][ind[k1-k2]]=rs2;
        ind[k1-k2]++;
        if (k1>k2)
        {
            shift[k1-k2-1][ind[k1-k2-1]]=-ls2;
            ind[k1-k2-1]++;
        }
    }

    if (B)
    {
        if (rs3==0) 
        {
            shift[k1-k3][ind[k1-k3]]=0;
            ind[k1-k3]++;
        }
        else
        {
            shift[k1-k3][ind[k1-k3]]=rs3;
            ind[k1-k3]++;
            if (k1>k3)
            {
                shift[k1-k3-1][ind[k1-k3-1]]=-ls3;
                ind[k1-k3-1]++;
            }
        }
        if (rs4==0) 
        {
            shift[k1-k4][ind[k1-k4]]=0;
            ind[k1-k4]++;
        }
        else
        {
            shift[k1-k4][ind[k1-k4]]=rs4;
            ind[k1-k4]++;
            if (k1>k4)
            {
                shift[k1-k4-1][ind[k1-k4-1]]=-ls4;
                ind[k1-k4-1]++;
            }
        }
    }

    for (i=0;i<64;i++)
    {
        if (ind[i]==0) continue;
        printf("        gx[%d]^=",i);
        xxor=0;
        for (j=0;j<ind[i];j++)
        {
            if (xxor) printf("^");
            if (shift[i][j]==0) {printf("top");  xxor++;}
            if (shift[i][j]>0) {printf("(top>>%d)",shift[i][j]); xxor++;}
            if (shift[i][j]<0) {printf("(top<<%d)",-shift[i][j]); xxor++;}
        }
        printf(";\n");
    }

    printf("        gx[%d]^=top;\n",k1-1);
    printf("        x->len=%d;\n",k1); 
    printf("        if (gx[%d]==0) mr_lzero(x);\n",k1-1);
    printf("        return;\n    }\n");
	return 0;
}
