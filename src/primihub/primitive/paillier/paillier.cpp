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


#include "paillier.h"
#include <cstdio>

Paillier::Paillier(int k, int l) : kSec(k), lSec(l) {
    irand((unsigned int)time(0));
    // init the secret key
    sk.alpha = mirvar(0);
    sk.miu = mirvar(0);
    sk.P2 = mirvar(0);
    sk.P2Inv = mirvar(0);
    sk.P = mirvar(0);
    sk.phiP2 = mirvar(0);
    sk.phiP = mirvar(0);
    sk.phiQ2 = mirvar(0);
    sk.phiQ = mirvar(0);
    sk.PInv = mirvar(0);
    sk.Q2 = mirvar(0);
    sk.Q2Inv = mirvar(0);
    sk.Q = mirvar(0);
    sk.QInv = mirvar(0);
    // init the public key
    pk.hs = mirvar(0);
    pk.N2 = mirvar(0);
    pk.N = mirvar(0);
    // for decryption
    sk._2p = mirvar(0);
    sk._2q = mirvar(0);
    sk.P2qInvQ = mirvar(0);
    sk.Q2pInvP = mirvar(0);
    sk.PInvP = mirvar(0);
}

void Paillier::keyGen() {
    // temporary variable
    big pt = mirvar(0), qt = mirvar(0);// are odd integers
    big p  = mirvar(0), q  = mirvar(0);// are odd primes
    big one = mirvar(1), two = mirvar(2);
    big gcdTmp = mirvar(0);
    do {
        // obtain strong prime p, q
        do {
            bigbits(lSec / 2, p);
            nxprime(p, p);
        } while (!isprime(p));
        do {
            bigbits(lSec / 2, q);
            nxprime(q, q);
        } while (!isprime(q));
        if (mr_compare(p, q) == 0) {
            continue;
        }
        // obtain pt, qt. n = k * 2
        bigbits((kSec * 2 - lSec) / 2 - 1, pt);
        if (divisible(pt, two)) {
            incr(pt, 1, pt);// pt is even, qt + 1 is odd
        }
        bigbits((kSec * 2 - lSec) / 2 - 1, qt);
        if (divisible(qt, two)) {
            incr(qt, 1, qt);// qt is even, qt + 1 is odd
        }
        if (egcd(p, pt, gcdTmp) != 1) {
            continue;
        }
        if (egcd(p, qt, gcdTmp) != 1) {
            continue;
        }
        if (egcd(q, pt, gcdTmp) != 1) {
            continue;
        }
        if (egcd(q, qt, gcdTmp) != 1) {
            continue;
        }
        if (egcd(pt, qt, gcdTmp) != 1) {
            continue;
        }
        // P = 2 * p * pt + 1
        multiply(p, pt, sk.P);
        premult(sk.P, 2, sk.P);
        incr(sk.P, 1, sk.P);
        if (!isprime(sk.P)) {
            continue;
        }
        // Q = 2 * q * qt + 1
        multiply(q, qt, sk.Q);
        premult(sk.Q, 2, sk.Q);
        incr(sk.Q, 1, sk.Q);
        if (!isprime(sk.Q)) {
            continue;
        }
        break;
    } while (true);
    // print to file
    //----------------------------------------------
    FILE* fp = fopen("./key/ant/pSec.txt", "w");
    cotnum(p, fp);
    fclose(fp);
    fp = fopen("./key/ant/pOdd.txt", "w");
    cotnum(pt, fp);
    fclose(fp);
    fp = fopen("./key/ant/qSec.txt", "w");
    cotnum(q, fp);
    fclose(fp);
    fp = fopen("./key/ant/qOdd.txt", "w");
    cotnum(qt, fp);
    fclose(fp);
    fp = fopen("./key/ant/P.txt", "w");
    cotnum(sk.P, fp);
    fclose(fp);
    fp = fopen("./key/ant/Q.txt", "w");
    cotnum(sk.Q, fp);
    fclose(fp);
    //----------------------------------------------
    mirkill(gcdTmp);
    // alpha = p * q
    multiply(p, q, sk.alpha);
    // beta = pt * qt
    big beta = mirvar(0);
    multiply(pt, qt, beta);
    // pre-computing 2 * alpha
    premult(sk.alpha, 2, sk.alpha);
    // pre-computing 2 * beta
    premult(beta, 2, beta);
    // calculate P^2, phi(P), phi(P^2)
    multiply(sk.P, sk.P, sk.P2);
    decr(sk.P, 1, sk.phiP);
    multiply(sk.phiP, sk.P, sk.phiP2);
    // calculate Q^2, phi(Q), phi(Q^2)
    multiply(sk.Q, sk.Q, sk.Q2);
    decr(sk.Q, 1, sk.phiQ);
    multiply(sk.phiQ, sk.Q, sk.phiQ2);
    // pInv = p^-1 mod q, qInv = q^-1 mod p
    xgcd(sk.P, sk.Q, sk.PInv, sk.PInv, sk.PInv);
    xgcd(sk.Q, sk.P, sk.QInv, sk.QInv, sk.QInv);
    // p2Inv = p2^-1 mod q2, q2Inv = q2^-1 mod p2
    xgcd(sk.P2, sk.Q2, sk.P2Inv, sk.P2Inv, sk.P2Inv);
    xgcd(sk.Q2, sk.P2, sk.Q2Inv, sk.Q2Inv, sk.Q2Inv);
    // N = P * Q
    multiply(sk.P, sk.Q, pk.N);
    multiply(pk.N, pk.N, pk.N2);
    // miu = alpha^-1 mod N
    xgcd(sk.alpha, pk.N, sk.miu, sk.miu, sk.miu);
    // y in ZN and y >= 1
    big y = mirvar(0);
    do {
        bigrand(pk.N, y);
    } while (mr_compare(y, one) < 0);
    // h = -y^(2 * beta) mod N
    powmod(y, beta, pk.N, y);
    negify(y, y);
    // hs = h^N mod N^2
    powmod(y, pk.N, pk.N2, pk.hs);
    // for decryption
    premult(p, 2, sk._2p);
    premult(q, 2, sk._2q);
    multiply(sk.Q, sk._2p, sk.Q2pInvP);
    xgcd(sk.Q2pInvP, sk.P, sk.Q2pInvP, sk.Q2pInvP, sk.Q2pInvP);
    multiply(sk.P, sk._2q, sk.P2qInvQ);
    xgcd(sk.P2qInvQ, sk.Q, sk.P2qInvQ, sk.P2qInvQ, sk.P2qInvQ);
    xgcd(sk.P, sk.Q, sk.PInvP, sk.PInvP, sk.PInvP);
    multiply(sk.PInvP, sk.P, sk.PInvP);
    //---------------------------------------------------------------
    mirkill(one);
    mirkill(two);
    mirkill(y);
    mirkill(beta);
    mirkill(p);
    mirkill(q);
    mirkill(pt);
    mirkill(qt);
}

void Paillier::keyRead() {
    // temporary variable
    big pt = mirvar(0), qt = mirvar(0);// are odd integers
    big p  = mirvar(0), q  = mirvar(0);// are odd primes
    big one = mirvar(1), two = mirvar(2);
    big gcdTmp = mirvar(0);
    // read from file
    //----------------------------------------------
    FILE* fp = fopen("./key/ant/pSec.txt", "r");
    cinnum(p, fp);
    fclose(fp);
    fp = fopen("./key/ant/pOdd.txt", "r");
    cinnum(pt, fp);
    fclose(fp);
    fp = fopen("./key/ant/qSec.txt", "r");
    cinnum(q, fp);
    fclose(fp);
    fp = fopen("./key/ant/qOdd.txt", "r");
    cinnum(qt, fp);
    fclose(fp);
    fp = fopen("./key/ant/P.txt", "r");
    cinnum(sk.P, fp);
    fclose(fp);
    fp = fopen("./key/ant/Q.txt", "r");
    cinnum(sk.Q, fp);
    fclose(fp);
    //----------------------------------------------
    mirkill(gcdTmp);
    // alpha = p * q
    multiply(p, q, sk.alpha);
    // beta = pt * qt
    big beta = mirvar(0);
    multiply(pt, qt, beta);
    // pre-computing 2 * alpha
    premult(sk.alpha, 2, sk.alpha);
    // pre-computing 2 * beta
    premult(beta, 2, beta);
    // calculate P^2, phi(P), phi(P^2)
    multiply(sk.P, sk.P, sk.P2);
    decr(sk.P, 1, sk.phiP);
    multiply(sk.phiP, sk.P, sk.phiP2);
    // calculate Q^2, phi(Q), phi(Q^2)
    multiply(sk.Q, sk.Q, sk.Q2);
    decr(sk.Q, 1, sk.phiQ);
    multiply(sk.phiQ, sk.Q, sk.phiQ2);
    // pInv = p^-1 mod q, qInv = q^-1 mod p
    xgcd(sk.P, sk.Q, sk.PInv, sk.PInv, sk.PInv);
    xgcd(sk.Q, sk.P, sk.QInv, sk.QInv, sk.QInv);
    // p2Inv = p2^-1 mod q2, q2Inv = q2^-1 mod p2
    xgcd(sk.P2, sk.Q2, sk.P2Inv, sk.P2Inv, sk.P2Inv);
    xgcd(sk.Q2, sk.P2, sk.Q2Inv, sk.Q2Inv, sk.Q2Inv);
    // N = P * Q
    multiply(sk.P, sk.Q, pk.N);
    multiply(pk.N, pk.N, pk.N2);
    // miu = alpha^-1 mod N
    xgcd(sk.alpha, pk.N, sk.miu, sk.miu, sk.miu);
    // y in ZN and y >= 1
    big y = mirvar(0);
    do {
        bigrand(pk.N, y);
    } while (mr_compare(y, one) < 0);
    // h = -y^(2 * beta) mod N
    powmod(y, beta, pk.N, y);
    negify(y, y);
    // hs = h^N mod N^2
    powmod(y, pk.N, pk.N2, pk.hs);
    // for decryption
    premult(p, 2, sk._2p);
    premult(q, 2, sk._2q);
    multiply(sk.Q, sk._2p, sk.Q2pInvP);
    xgcd(sk.Q2pInvP, sk.P, sk.Q2pInvP, sk.Q2pInvP, sk.Q2pInvP);
    multiply(sk.P, sk._2q, sk.P2qInvQ);
    xgcd(sk.P2qInvQ, sk.Q, sk.P2qInvQ, sk.P2qInvQ, sk.P2qInvQ);
    xgcd(sk.P, sk.Q, sk.PInvP, sk.PInvP, sk.PInvP);
    multiply(sk.PInvP, sk.P, sk.PInvP);
    //---------------------------------------------------------------
    mirkill(one);
    mirkill(two);
    mirkill(y);
    mirkill(beta);
    mirkill(p);
    mirkill(q);
    mirkill(pt);
    mirkill(qt);
}

void Paillier::encryptionCrtPrepare() {
    big hsP = mirvar(0), hsQ = mirvar(0), modTmp = mirvar(0);
    copy(pk.hs, hsP);
    divide(hsP, sk.P2, modTmp);
    copy(pk.hs, hsQ);
    divide(hsQ, sk.Q2, modTmp);
    brick_init(&encBrickP2, hsP, sk.P2, 19, lSec + 1);
	brick_init(&encBrickQ2, hsQ, sk.Q2, 19, lSec + 1);
    mirkill(modTmp);
    mirkill(hsP);
    mirkill(hsQ);
}

big Paillier::encryptionCrt(const big& m) {
    big c = mirvar(0), lhs = mirvar(0);
    // lhs = (1 + mN)
    multiply(m, pk.N, lhs);
    incr(lhs, 1, lhs);
    // r in {0, 1}^l
    big r = mirvar(0), rhs = mirvar(0);
    bigbits(lSec, r);
    // rhs = hs^r mod N^2
    //------------------------------
    big cp = mirvar(0), cq = mirvar(0);
	pow_brick(&encBrickP2, r, cp);
	pow_brick(&encBrickQ2, r, cq);
	subtract(cq, cp, cq);// cq = cq - cp
	multiply(cq, sk.P2, cq);
	prepare_monty(pk.N2);
	big nresL = mirvar(0), nresR = mirvar(0), nresRes = mirvar(0);
	nres(cq, nresL);
	nres(sk.P2Inv, nresR);
	nres_modmult(nresL, nresR, nresRes);
	nres(cp, nresL);
	nres_modadd(nresL, nresRes, nresRes);// nresRes => rhs
	mirkill(cp);
	mirkill(cq);
    //------------------------------
    nres(lhs, nresL);
    nres_modmult(nresL, nresRes, nresRes);
    redc(nresRes, c);
    mirkill(nresL);
	mirkill(nresR);
	mirkill(nresRes);
    mirkill(lhs);
    mirkill(r);
    mirkill(rhs);
    return c;
}

void Paillier::encryptionCrtEnd() {
    brick_end(&encBrickP2);
    brick_end(&encBrickQ2);
}

big Paillier::decryptionCrt(const big& c) {
    big m = mirvar(0), t = mirvar(0);
    // t = c^alpha mod N^2
    NumTools::task_powmod_crt(c, sk.alpha, 
			sk.P2, sk.Q2, pk.N2, sk.P2Inv, t);
    // L(t) = (t - 1) / N
    decr(t, 1, t);
    divide(t, pk.N, t);
    NumTools::modmul(t, sk.miu, pk.N, m);
    mirkill(t);
    return m;
}

big Paillier::decryptionPaiCrt(const big& c) {
    big m = mirvar(0), cp = mirvar(0), cq = mirvar(0);
    big nresL = mirvar(0), nresR = mirvar(0);
    NumTools::sub_task_powmod(c, sk._2p, sk.P2, cp);
    decr(cp, 1, cp);
    divide(cp, sk.P, cp);
    multiply(cp, sk.Q2pInvP, cp);
    NumTools::sub_task_powmod(c, sk._2q, sk.Q2, cq);
    decr(cq, 1, cq);
    divide(cq, sk.Q, cq);
    multiply(cq, sk.P2qInvQ, cq);
    subtract(cq, cp, cq);
    prepare_monty(pk.N);
    nres(cq, nresL);
    nres(sk.PInvP, nresR);
    nres_modmult(nresL, nresR, m);
    nres(cp, nresL);
    nres_modadd(nresL, m, m);
    redc(m, m);
    mirkill(cp);
    mirkill(cq);
    mirkill(nresL);
    mirkill(nresR);
    return m;
}

Paillier::~Paillier() {
    mirkill(sk.alpha);
    mirkill(sk.miu);
    mirkill(sk.P2);
    mirkill(sk.P2Inv);
    mirkill(sk.P);
    mirkill(sk.phiP2);
    mirkill(sk.phiP);
    mirkill(sk.phiQ2);
    mirkill(sk.phiQ);
    mirkill(sk.PInv);
    mirkill(sk.Q2);
    mirkill(sk.Q2Inv);
    mirkill(sk.Q);
    mirkill(sk.QInv);
    mirkill(pk.hs);
    mirkill(pk.N2);
    mirkill(pk.N);
    // for decryption
    mirkill(sk._2p);
    mirkill(sk._2q);
    mirkill(sk.Q2pInvP);
    mirkill(sk.P2qInvQ);
    mirkill(sk.PInvP);
    //---------------------------------------------------------------
}
