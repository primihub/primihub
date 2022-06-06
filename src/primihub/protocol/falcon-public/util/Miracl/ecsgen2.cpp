/*
 *   Proposed Digital Signature Standard
 *
 *   Elliptic Curve Variation GF(2^m) - See Dr. Dobbs Journal April 1997
 *
 *   This program generates one set of public and private keys in files 
 *   public.ecs and private.ecs respectively. Notice that the public key 
 *   can be much shorter in this scheme, for the same security level.
 *
 *   It is assumed that Curve parameters are to be found in file common2.ecs
 *
 *   The curve is y^2+xy = x^3+Ax^2+B over GF(2^m) using a trinomial or 
 *   pentanomial basis (t^m+t^a+1 or t^m+t^a+t^b+t^c+1). These parameters
 *   can be generated using the findbase.cpp example program, or taken from tables
 *   provided, for example in IEEE-P1363 Annex A
 *
 *   The file common2.ecs is presumed to exist and contain 
 *   {m,A,B,q,x,y,a,b,c} where A and B are parameters of the equation 
 *   above, (x,y) is an initial point on the curve, {m,a,b,c} are the field 
 *   parameters, (b is zero for a trinomial) and q is the order of the 
 *   (x,y) point, itself a large prime. The number of points on the curve is 
 *   cf.q where cf is the "co-factor", normally 2 or 4.
 * 
 *   Requires: big.cpp ec2.cpp
 */

#include <iostream>
#include <fstream>
#include "ec2.h"

using namespace std;

Miracl precision=20;

int main()
{
    ifstream common("common2.ecs");    /* construct file I/O streams */
    ofstream public_key("public.ecs");
    ofstream private_key("private.ecs");
    int ep,m,a,b,c;
    EC2 G,W;
    Big a2,a6,q,x,y,d;
    long seed;
    miracl *mip=&precision;

    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);

    common >> m;
    mip->IOBASE=16;
    common >> a2 >> a6 >> q >> x >> y;
    mip->IOBASE=10;
    common >> a >> b >> c;

    ecurve2(m,a,b,c,a2,a6,FALSE,MR_PROJECTIVE);

    if (!G.set(x,y))
    {
        cout << "Problem - point (x,y) is not on the curve" << endl;
        return 0;
    }

    W=G;
    W*=q;
    if (!W.iszero())
    {
        cout << "Problem - point (x,y) is not of order q" << endl;
        return 0;
    }

/* generate public/private keys */

    d=rand(q);
    G*=d;
    ep=G.get(x);
    cout << "public key = " << ep << " " << x << endl;
    public_key << ep << " " << x << endl;
    private_key << d << endl;
    return 0;
}

