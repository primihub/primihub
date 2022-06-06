/*
 * Solve set of linear equations involving
 * a Hilbert matrix
 * i.e. solves   Hx=b, where b is the vector [1,1,1....1]
 *
 * Requires: flash.cpp
 *
 */

#include <iostream>
#include "flash.h"

using namespace std;

Miracl precision=20;

static Flash A[20][20];
static Flash b[20];

BOOL gauss(Flash A[][20],Flash b[],int n)
{ /* solve Ax=b using Gaussian elimination *
   * solution returned in b                */ 
    int i,ii,j,jj,k,m;
    int row[20];
    BOOL ok;
    Flash s;
    ok=TRUE;
    for (i=0;i<n;i++) {A[i][n]=b[i];row[i]=i;}
    for (i=0;i<n;i++)
    { /* Gaussian elimination */
        m=i; ii=row[i];

        s=fabs(A[ii][i]);
        for (j=i+1;j<n;j++) 
        { // find best pivot
            jj=row[j];
            if (fabs(A[jj][i])>s) 
            {
                m=j; 
                s=fabs(A[jj][i]);
            } 
        }

        if (s==0)
        { // no non-zero pivot found
            ok=FALSE;
            break;
        }

        k=row[i]; row[i]=row[m]; row[m]=k;  // swap row indices

        ii=row[i];        
        for (j=i+1;j<n;j++)
        {
            jj=row[j];
            s=A[jj][i]/A[ii][i];
            for (k=n;k>=i;k--)  A[jj][k]-=s*A[ii][k];   
        }  
    }
    if (ok) for (j=n-1;j>=0;j--)
    { /* Backward substitution */
        s=0;
        for (k=j+1;k<n;k++)  s+=b[k]*A[row[j]][k];
        if (A[row[j]][j]==0)
        {
            ok=FALSE;
            break;
        } 
        b[j]=(A[row[j]][n]-s)/A[row[j]][j];
    }
    return ok;
}

int main()
{ /* solve set of linear equations */
    int i,j,n;
    miracl *mip=&precision;
    mip->RPOINT=OFF;   /* use fractions for output */
    do
    {
        cout << "Order of Hilbert matrix H= ";
        cin >> n;
    } while (n<2 || n>19);
    for (i=0;i<n;i++)
    {
        b[i]=1;
        for (j=0;j<n;j++)
              A[i][j]=(Flash)1/(i+j+1);  
    }
    if (gauss(A,b,n))
    {
        cout << "\nSolution is\n";
        for (i=0;i<n;i++)  cout << "x[" << i+1 << "] = " << b[i] << "\n";
        if (mip->EXACT) cout << "Result is exact!\n";
    }
    else cout <<  "H is singular!\n";

    return 0;
}

