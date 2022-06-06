/*
 * Solve set of linear equations involving
 * a Hilbert matrix
 * i.e. solves   Hx=b, where b is the vector [1,1,1....1]
 *
 */

#include <stdio.h>
#include "miracl.h"

static flash AA[50][50];
static flash bb[50];

BOOL gauss(flash A[][50],flash b[],int n)
{ /* solve Ax=b using Gaussian elimination *
   * solution x returned in b              */
    int i,j,k,m;
    BOOL ok;
    flash w,s;
    w=mirvar(0);
    s=mirvar(0);
    ok=TRUE;
    for (i=0;i<n;i++)
        copy(b[i],A[i][n]);
    for (i=0;i<n;i++)
    { /* Gaussian elimination */
        m=i;
        for (j=i+1;j<n;j++)
        {
            absol(A[j][i],w);
            absol(A[m][i],s);
            if (fcomp(w,s)>0) m=j;
        }
        if (m!=i) for (k=i;k<=n;k++)
        {
            copy(A[i][k],w);
            copy(A[m][k],A[i][k]);
            copy(w,A[m][k]);
        }
        if (size(A[i][i])==0)
        {
            ok=FALSE;
            break;
        }
        for (j=i+1;j<n;j++)
        {
            fdiv(A[j][i],A[i][i],s);

            for (k=n;k>=i;k--)
            {
                fmul(s,A[i][k],w);
                fsub(A[j][k],w,A[j][k]);
            }
        }
    }
    if (ok) for (j=n-1;j>=0;j--)
    { /* Backward substitution */
        zero(s);
        for (k=j+1;k<n;k++)
        {
            fmul(A[j][k],b[k],w);
            fadd(s,w,s);
        }
        fsub(A[j][n],s,w);
        if (size(A[j][j])==0)
        {
            ok=FALSE;
            break;
        } 
        fdiv(w,A[j][j],b[j]);
    }
    mirkill(s);
    mirkill(w);
    return ok;
}

int main()
{ /* solve set of linear equations */
    int i,j,n;
    miracl *mip=mirsys(20,MAXBASE);
    do
    {
        printf("Order of Hilbert matrix H= ");
        scanf("%d",&n);
        getchar();
    } while (n<2 || n>49);
    for (i=0;i<n;i++)
    {
        AA[i][n]=mirvar(0);
        bb[i]=mirvar(1);
        for (j=0;j<n;j++)
        {
            AA[i][j]=mirvar(0);
            fconv(1,i+j+1,AA[i][j]);
        }
    }
    
    if (gauss(AA,bb,n))
    {
        printf("\nSolution is\n");
        for (i=0;i<n;i++)
        {
            printf("x[%d] = ",i+1);
            cotnum(bb[i],stdout);
        }
        if (mip->EXACT) printf("Result is exact!\n");
    }
    else printf("H is singular!\n");
    return 0;
}

