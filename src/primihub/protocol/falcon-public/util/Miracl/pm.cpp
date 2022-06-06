/*
 * Simple program to find pseudo-mersenne primes 
 */

#include <iostream>
#include "big.h"

using namespace std;

Miracl precision=100;

int main()
{
	int i,w;
	Big p;
	
	for (i=112;i<=256;i+=8)
	{
		p=pow((Big)2,i);
		p=p-1; w=1;
		while (p%4!=3 || !prime(p)) {p-=1; w++;}
		cout << "2^" << i << "-" << w << " is a prime" << endl;
	}

	return 0;
}
