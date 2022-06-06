/*
 *   Program to encode text using RSA public key.
 *
 *   Note that an exponent of 3 must be used with extreme care!
 *
 *   For example:-
 *   If the same message m is encrypted under 3 or more different public keys
 *   the message can be recovered without factoring the modulus (using the
 *   Chinese remainder thereom).
 *   If the messages m and m+1 are encrypted under the same public key, again 
 *   the message can be recovered without factoring the modulus. Indeed any
 *   simple relationship between a pair of messages can be exploited in a
 *   similar way. 
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"   /* include MIRACL system */

using namespace std;

Miracl precision=100;

void strip(char *name)
{ /* strip off filename extension */
    int i;
    for (i=0;name[i]!='\0';i++)
    {
        if (name[i]!='.') continue;
        name[i]='\0';
        break;
    }
}

int main()
{  /*  encode using public key  */
    Big e,m,y,ke,mn,mx;
    ifstream public_key("public.key");
    ifstream input_file;
    ofstream output_file;
    static char line[500];
    static char buff[256];
    static char ifname[13],ofname[13];
    BOOL fli,last;
    int i,ipt,klen;
    miracl *mip=&precision;
    mip->IOBASE=16;
    public_key >> ke;
    mn=root(ke,3);
    mx=mn*mn*mn-mn*mn;
 /* find key length in characters */
    klen=bits(mx)/7;
    cout << "file to be encoded = " ;
    cin.getline(ifname,13);
    fli=FALSE;
    if (strlen(ifname)>0) fli=TRUE;
    if (fli)
    { /* set up input file */
        strcpy(ofname,ifname);
        strip(ofname);
        strcat(ofname,".rsa");
        input_file.open(ifname,ios::in); 
        if (!input_file)
        {
            cout << "Unable to open file " << ifname << "\n";
            return 0;
        }
        cout << "encoding message\n";
    }
    else
    { /* accept input from keyboard */
        cout << "output filename = ";
        cin >> ofname; 
        strip(ofname);    
        strcat(ofname,".rsa");
        cout << "input message - finish with cntrl z\n";
    }
    output_file.open(ofname);
    ipt=0;
    last=FALSE;
    while (!last)
    { /* encode line by line */
        if (fli)
        {
            input_file.getline(&line[ipt],132);
            if (input_file.eof() || input_file.fail()) last=TRUE;
        }
        else
        {
            cin.getline(&line[ipt],132);
            if (cin.eof() || cin.fail()) last=TRUE;
        }
        strcat(line,"\n");
        ipt=strlen(line);
        if (ipt<klen && !last) continue;
        while (ipt>=klen)
        { /* chop up into klen-sized chunks and encode */
            for (i=0;i<klen;i++)
                buff[i]=line[i];
            buff[klen]='\0';
            for (i=klen;i<=ipt;i++)
                line[i-klen]=line[i];
            ipt-=klen;
            mip->IOBASE=128;
            m=buff;
            e=pow(m,3,ke);
            mip->IOBASE=16;
            output_file << e << endl;
        }
        if (last && ipt>0)
        { /* now deal with left overs */
            mip->IOBASE=128;
            m=line;
            if (m<mn)
            { /* pad out with random number if necessary */
                m+=mn*(mn*mn-rand(mn));
            }
            e=pow(m,3,ke);
            mip->IOBASE=16;
            output_file << e << endl;
        }
    }
    return 0;
}   

