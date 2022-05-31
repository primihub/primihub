/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */


#ifndef __PAILLIER__
#define __PAILLIER__

#include "numTools.h"
#include <ctime>

struct PaillierPK {
    big N, hs;
    // pre-computing for encryption
    big N2;
};

struct PaillierSK {
    big alpha, miu;
    // pre-computing for crt
    // for mod N^2
    big P2, Q2;
    big P2Inv, Q2Inv;
    big phiP2, phiQ2;
    // for mod N
    big P, Q;
    big PInv, QInv;
    big phiP, phiQ;
    // for decryption
    big _2p, _2q;
    big Q2pInvP, P2qInvQ;
    big PInvP;
};

class Paillier {
private:
    PaillierSK sk;
    brick encBrickP2, encBrickQ2;
    int kSec, lSec;

public:
    PaillierPK pk;

public:
    void encryptionCrtPrepare();

    void encryptionCrtEnd();

public:
    Paillier(int k, int l);

    void keyGen();

    void keyRead();

    ~Paillier();

    big encryptionCrt(const big& m);

    big decryptionCrt(const big& c);

    big decryptionPaiCrt(const big& c);

};

#endif
