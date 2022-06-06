/*
   Waters ABE 
   "Ciphertext-Policy Attribute-Based Encryption: An Expressive, Efficient, and Provably Secure Realisation"
   Section 3

   Implemented on Type-3 pairing

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX cpabe.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX cpabe.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX cpabe.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX cpabe.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX cpabe.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   See http://eprint.iacr.org/2008/290
*/

#include <iostream>
#include <ctime>

//********* choose just one of these pairs **********
//#define MR_PAIRING_CP      // AES-80 security   
//#define AES_SECURITY 80

//#define MR_PAIRING_MNT	// AES-80 security
//#define AES_SECURITY 80

//#define MR_PAIRING_BN    // AES-128 or AES-192 security
//#define AES_SECURITY 128
//#define AES_SECURITY 192

//#define MR_PAIRING_KSS    // AES-192 security
//#define AES_SECURITY 192

#define MR_PAIRING_BLS    // AES-256 security
#define AES_SECURITY 256
//*********************************************

#include "pairing_3.h"

// Access structure - crude boolean description of combination of attributes that a recipient must have
// to be able to reconstruct the secret value

/* Note that Access structure consists of threshold gates - output is TRUE if t out of n inputs are TRUE  */
/* Access tree: Number (n), threshold (t), followed by n Child nodes */
/* Negative number denotes leaf node */

/*
// This is example from Liu and Cao http://eprint.iacr.org/2010/374 
// Negative number denotes leaf node

int Access[]={           //   node - index
2,2,1,2,                 // node 0 - 0  Here (n,t) = (2,2) so AND gate and the two children are nodes 1 and 2 
2,1,3,4,                 // node 1 - 4  Here (n,t) = (2,1) so OR gate and the two children are nodes 3 and 4 
4,3,-'E',-'F',-'G',-'H', // node 2 - 8  
2,2,-'B',5,              // node 3 - 14 Here (n,t) = (2,2) the first child is a leaf node, the second is node 5 
3,2,-'C',-'D',-'E',      // node 4 - 18 Here (n,t) = (3,2) and all 3 children are leaf nodes.
2,1,-'A',-'C',           // node 5 - 23
0};

// Note total of 8 attributes A-H

#define U 8

// These are the attributes of a particular recipient

int auth[]={'D','E','F','G',0}; // attributes of recipient
*/

int Access[]={           //   node - index
3,3,1,2,6,               // node 0 - 0  Here (n,t) = (3,3) so AND gate and the children are nodes 1 and 2 and 6
2,1,3,4,                 // node 1 - 4  Here (n,t) = (2,1) so OR gate and the two children are nodes 3 and 4 
4,3,-'E',-'F',-'G',-'H', // node 2 - 8  
2,2,-'B',5,              // node 3 - 14 Here (n,t) = (2,2) the first child is a leaf node, the second is node 5 
3,2,-'C',-'D',-'E',      // node 4 - 18 Here (n,t) = (3,2) and all 3 children are leaf nodes.
2,1,-'A',-'C',           // node 5 - 23
2,1,-'I',7,				 // add in n<11 condition
2,2,-'J',8,
2,1,-'K',-'L',
0};

// Note total of 12 attributes A-L

#define U 12

// These are the attributes of a particular recipient

int auth[]={'D','E','F','G','J','K',0}; // attributes of valid recipient - try removing one!


/*
int Access[]={           //   node - index
20,20,-'A',-'B',-'C',-'D',-'E',-'F',-'G',-'H',-'I',-'J',-'K',-'L',-'M',-'N',-'O',-'P',-'Q',-'R',-'S',-'T',0
};

#define U 20

int auth[]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T',0};
*/

// get node index for i-th node

int find_index(int Access[],int i)
{
	int n,j,k;
	j=k=0;
	while (k<i)
	{
		n=Access[j];
		if (n==0) return -1;
		j+=(n+2);
		k++;
	}
	return j;
}

// simple class for Access tree

class node
{
public:
	int n;
	int t;
	node *child;

	node() {n=t=0; child=NULL;}
	node& operator=(node&); 
	node(int *);  // construct from access structure
	friend ostream& operator<<(ostream&, node&);
    ~node();
};

// fill tree data structure from data

void fill_node(node *nd,int *Access, int ipos)
{
	int i,j,k;
	nd->n=Access[ipos];

	nd->t=Access[ipos+1];
	
	nd->child=new node [nd->n];
	for (i=0;i<nd->n;i++)
	{
		k=Access[ipos+2+i];
		if (k<0) nd->child[i].n=k;
		else
		{
			j=find_index(Access,k);
			fill_node(&nd->child[i],Access,j);
		}
	}
}

void delete_node(node *nd)
{
	int i,n=nd->n;
	nd->n=0;
	nd->t=0;
	if (n<=0) return;
	for (i=0;i<n;i++)
		delete_node(&nd->child[i]);
	delete [] nd->child;
	nd->child=NULL;
}

void copy_node(node *f, node *t)
{
	int i;
	t->n=f->n;
	if (t->n<=0)	return;
	t->t=f->t;
	t->child=new node [t->n];
	for (i=0;i<t->n;i++)
		copy_node(&f->child[i],&t->child[i]);
}

// node constructor

node::node(int *Access)
{
	fill_node(this,Access,0);
}

// node destructor

node::~node()
{
	delete_node(this);
}

node& node::operator=(node& b)
{
	delete_node(this);
	copy_node(&b,this);
	return *this;
}

// Find number of leaf attributes - the number of rows in Access matrix

int find_m(int Access[])
{
	int i,m;
	i=m=0;
	while (Access[i]!=0)
	{
		i++;
		if (Access[i]<0) m++;
	}
	return m;
}

// find number of columns in Access matrix = 1 + Sum of (thresholds-1) ??

int find_d(int Access[])
{
	int j,d,n;
	j=0; d=1;
	while (Access[j]>0)
	{
		n=Access[j];
		if (n==0) break;
		d+=Access[j+1]-1;
		j+=(n+2);

	}
	return d;
}

// traverse Access tree and pretty-print it

void print_node(node *nd)
{
	int c,n=nd->n;
	if (n<0) 
	{
		cout << "(" << (char)(-n) << ")";
		return;
	}
	cout << "(";
	for (int i=0;i<n;i++)
	{
		c=nd->child[i].n;
		if (c<0)
			cout << (char)(-c) << ",";
		else	
			print_node(&nd->child[i]);
	}
	cout << nd->t << "),";
}

ostream& operator<<(ostream& s, node& x)
{
    print_node(&x);
    return s;
}

//
// make LSSS matrix of size rowsXcols from Access description
// attr[i] contains attribute of each row.
// algorithm due to Liu and Cao http://eprint.iacr.org/2010/374 
// (but much simplified)
//

void make_LSSS(int Access[],int rows,int cols,Big **LSSS,int *attr)
{
	int i,j,z,m,d,n,t;
	Big k;
	node Fz,root(Access);

	node *L=new node[rows];
	
	for (i=0;i<rows;i++)
	{
		for (j=0;j<cols;j++)
			LSSS[i][j]=0;
	}

	LSSS[0][0]=1; L[0]=root; m=1; d=1;

	for(;;)
	{
		z=-1;
		for (i=0;i<m;i++)
		{
			if (L[i].n<0) continue;
			z=i;
			break;
		}
		if (z<0) break;
		Fz=L[z];
		n=Fz.n;
		t=Fz.t;
		for (i=m-1;i>=z;i--)
			L[i+n-1]=L[i];
		
		for (i=0;i<n;i++)
			L[z+i]=Fz.child[i];
		
		for (i=m-1;i>=z;i--)
		{
			for (j=0;j<d;j++)
				LSSS[i+n-1][j]=LSSS[i][j];
		}
		for (i=0;i<n;i++)
		{
			k=1;
			for (j=0;j<d;j++)
				LSSS[z+i][j]=LSSS[z][j];
			for (j=1;j<t;j++)
			{
				k*=(i+1);
				LSSS[z+i][d+j-1]=k;
			}
		}
		m=m+n-1;
		d=d+t-1;	
	}

	for (i=0;i<m;i++) attr[i]=-L[i].n;
	delete [] L;
}

// Given Aw=I, where I=(1,0,0...,0), returns w
// where A is nxn matrix and w in an n element vector

BOOL gauss(Big &order,int n,Big *matrix[],Big *w)
{
    int i,ii,j,jj,k,m;
    int *row;
    BOOL ok,pivot;
    Big s,p;
    ok=TRUE;
	w[0]=1;
	row=new int[n];
	for (i=1;i<n;i++) w[i]=0;
    for (i=0;i<n;i++) {matrix[i][n]=w[i];row[i]=i;}	
    for (i=0;i<n;i++)
    { /* Gaussian elimination */
        m=i; ii=row[i];

		pivot=TRUE;
		if (matrix[ii][i]==0)
		{ // look for non-zero pivot..
			pivot=FALSE;
			for (j=i+1;j<n;j++)
			{
				jj=row[j];
				if (matrix[jj][i]!=0)
				{
					m=j;
					pivot=TRUE;
					k=row[i]; row[i]=row[m]; row[m]=k;  // swap row indices
					break;
				}
			}
		}

        if (!pivot)
        { // no non-zero pivot found
            ok=FALSE;
            break;
        }

        ii=row[i];   
		p=inverse(matrix[ii][i],order);
        for (j=i+1;j<n;j++)
        {
            jj=row[j];
            s=modmult(matrix[jj][i],p,order);
            for (k=n;k>=i;k--)  matrix[jj][k]-=modmult(s,matrix[ii][k],order);   
        }  
    }
    if (ok) for (j=n-1;j>=0;j--)
    { /* Backward substitution */
        s=0;
        for (k=j+1;k<n;k++)  s+=modmult(w[k],matrix[row[j]][k],order);
        if (matrix[row[j]][j]==0)
        {
            ok=FALSE;
            break;
        } 
        w[j]=moddiv(matrix[row[j]][n]-s,matrix[row[j]][j],order);
// try and make them small
		if (abs(w[j])>order/2)
		{
			if (w[j]<0)      w[j]+=order;
			else if (w[j]>0) w[j]-=order;
		}
    }
	delete [] row;
    return ok;
}

// given set of attributes and LSSS matrix, returns reconstruction constant numerators w, and rows
// Note that original LSSS matrix is destroyed. Returns denominator of w.

BOOL reduce_LSSS(Big &order,int &m,int &d,Big **LSSS,int *attr,int *auth,int *rows,Big *w)
{
    int i,j,k,n,nattr=0;
	Big s,det;
    while (auth[nattr]!=0) nattr++;

// find rows in LSSS which are associated with attributes in auth
	k=0;
	for (i=0;i<m;i++)
	{
		for (j=0;j<nattr;j++)
		{
			if (attr[i]==auth[j])
			{
				rows[k++]=i;
				break;
			}
		}
	}
	m=k;

// find active rows of LSSS, and remove redundant ones
	for (i=0;i<m;i++)
	{
		if (rows[i]==i) continue;
		for (j=0;j<d;j++)
			LSSS[i][j]=LSSS[rows[i]][j];
		attr[i]=attr[rows[i]];
	}

// remove redundant columns of all zeros from LSSS
	int nzs;
	for (j=0;j<d;j++)
	{
		nzs=0;
		for (i=0;i<m;i++)
		{
			if (LSSS[i][j]!=0) {nzs++; break;}
		}
		if (nzs!=0) continue;
		d--;
		for (i=0;i<m;i++)
		{
			for (n=j;n<d;n++)
				LSSS[i][n]=LSSS[i][n+1];
		}
	}
	if (m<d) return FALSE; // unable to reconstruct

// calculate reconstruction constants

// transpose matrix
	for (i=0;i<m;i++)
	{
		for (j=i+1;j<m;j++)
		{
			s=LSSS[i][j]; LSSS[i][j]=LSSS[j][i]; LSSS[j][i]=s; 
		}
	}

	if (!gauss(order,m,LSSS,w)) return FALSE;
	return TRUE;
}

int main()
{
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve
	miracl *mip=get_mip();  // get handle on mip (Miracl Instance Pointer)
	time_t seed;
	int i,j,k,l,n,m,d,nS;
	Big **LSSS;
	int *attr;
	Big det,order=pfc.order();  // get pairing-friendly group order

	Big alpha,a,M,CT,s,t;
	G2 g2,g2a,*h,CD;
	G1 g1,g1a,K,L,MSK;
	GT eg2g1a,mask;

	time(&seed);       // initialise (insecure!) random numbers
    irand((long)seed);

// Setup

	pfc.random(alpha);
	pfc.random(a);
	pfc.random(g1);
	pfc.random(g2);
	pfc.precomp_for_mult(g1);  // g1 is fixed, so precompute on it
	pfc.precomp_for_mult(g2);  // g2 is fixed, so precompute on it

	eg2g1a=pfc.power(pfc.pairing(g2,g1),alpha);
	pfc.precomp_for_power(eg2g1a);

	g1a=pfc.mult(g1,a);
	g2a=pfc.mult(g2,a);
	pfc.precomp_for_mult(g2a);  // g2a is fixed, so precompute on it

	h=new G2[U]; 
	for (i=0;i<U;i++)
	{
		pfc.random(h[i]);
		pfc.precomp_for_mult(h[i]);  // precompute on h[.]
	}
	MSK=pfc.mult(g1,alpha);

// Encrypt

	m=find_m(Access);
	d=find_d(Access);

	LSSS=new Big *[m];
	for (i=0;i<m;i++)
		LSSS[i]=new Big[d+1]; // get memory for each row
	attr=new int [m];

	make_LSSS(Access,m,d,LSSS,attr); // create LSSS matrix from Access structure
	
	cout << "LSSS matrix " << m << "x" << d << endl;
	for (i=0;i<m;i++)
	{
		for (j=0;j<d;j++)
			cout << LSSS[i][j] << " ";
		cout << "    " << (char)attr[i] << endl;
	}
	cout << endl;

	mip->IOBASE=256;
	M=(char *)"test message"; 
	cout << "Message to be encrypted=   " << M << endl;
	mip->IOBASE=16;

	Big *v=new Big [d];
	Big *lambda=new Big [m];
	Big *r=new Big [m];
	for (i=0;i<d;i++)
		pfc.random(v[i]);
	s=v[0];
	for (i=0;i<m;i++)
		pfc.random(r[i]);
	
	G2 *C=new G2 [m];
	G1 *D=new G1 [m];

	for (i=0;i<m;i++)
	{
		lambda[i]=0;
		for (j=0;j<d;j++)
			lambda[i]+=modmult(LSSS[i][j],v[j],order);
		lambda[i]%=order;
	}

	CT=lxor(M,pfc.hash_to_aes_key(pfc.power(eg2g1a,s)));
	CD=pfc.mult(g2,s);

	for (i=0;i<m;i++)
	{
		C[i]=pfc.mult(g2a,lambda[i])+pfc.mult(h[attr[i]-'A'],-r[i]);
		D[i]=pfc.mult(g1,r[i]);
	}

// keygen
	cout << "Generating keys" << endl;
	pfc.random(t);
	K=MSK+pfc.mult(g1a,t);
	L=pfc.mult(g1,t);

	G2 *KX=new G2[U];
	nS=0;
	while (auth[nS]!=0)
	{
		j=auth[nS++]-'A';
		KX[j]=pfc.mult(h[j],t);
		pfc.precomp_for_pairing(KX[j]);
	}

// decrypt - and apply some optimisations e.g. e(A,B)*e(A,C) = e(A,B+C)
	cout << "Decrypting" << endl;
    int *rows=new int [m];
	Big *w=new Big [m];

	if (!reduce_LSSS(order,m,d,LSSS,attr,auth,rows,w)) 
	{
		cout << "Unable to decrypt - insufficient attributes" << endl;
		exit(0);
	}
	cout << "reduced matrix= " << m << "x" << d << endl;

// Note that w[i] are usually very small, so this is fast
	
	G2 TC;
	for (i=0;i<m;i++)
	{
		cout << "w[i]=  " << w[i] << endl;
		TC=TC+pfc.mult(C[rows[i]],w[i]);
		D[rows[i]]=pfc.mult(D[rows[i]],-w[i]);
	}
	TC=-TC;

// combine rows which share same attribute

	for (i=0;i<m;i++)
	{
		k=rows[i];
		for (j=i+1;j<m;j++)
		{
			if (attr[j]==attr[i])
			{
				D[k]=D[k]+D[rows[j]];  // combine them
				for (n=j;n<m;n++)
				{
					rows[n]=rows[n+1];
					attr[n]=attr[n+1];
				}
				m--;  // delete row
			}
		}
	}

	G2 **t2=new G2* [m+2];
	G1 **t1=new G1* [m+2];

	t2[0]=&CD; t1[0]=&K;  // e(CD,K)
	t2[1]=&TC; t1[1]=&L;  // e(TC,L)
	for (i=0;i<m;i++)
	{
		t2[i+2]=&KX[attr[i]-'A'];
		t1[i+2]=&D[rows[i]];
	}

	mask=pfc.multi_pairing(m+2,t2,t1);  // most pairings benefit from precomputation

	M=lxor(CT,pfc.hash_to_aes_key(mask));

	mip->IOBASE=256;
	cout << "Decrypted message=         " << M << endl;

	return 0;
}
