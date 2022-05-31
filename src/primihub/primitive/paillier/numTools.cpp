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


#include "numTools.h"
#include <chrono>

void NumTools::modmul
(const big& x, const big& y, const big& n, big& mul) {
	prepare_monty(n);
	big nresX = mirvar(0), nresY = mirvar(0);
	nres(x, nresX);
	nres(y, nresY);
	nres_modmult(nresX, nresY, mul);
	redc(mul, mul);// normalize
	mirkill(nresX);
	mirkill(nresY);
}

void NumTools::sub_task_powmod
(const big& a, const big& n, const big& p, big& res) {
	prepare_monty(p);
	big nresA = mirvar(0);
	nres(a, nresA);
	nres_powmod(nresA, n, res);
	redc(res, res);
	mirkill(nresA);
}

void NumTools::task_powmod_brick_crt(const big& n, const big& pInv, brick* bp, brick* bq
, const big& p, const big& q, const big& pq, big& res) {
	big cp = mirvar(0), cq = mirvar(0);
	pow_brick(bp, n, cp);
	pow_brick(bq, n, cq);
	subtract(cq, cp, cq);// cq = cq - cp
	multiply(cq, p, cq);
	prepare_monty(pq);
	big nresL = mirvar(0), nresR = mirvar(0), nresRes = mirvar(0);
	nres(cq, nresL);
	nres(pInv, nresR);
	nres_modmult(nresL, nresR, nresRes);
	nres(cp, nresL);
	nres_modadd(nresL, nresRes, nresRes);
	redc(nresRes, res);
	mirkill(nresL);
	mirkill(nresR);
	mirkill(nresRes);
	mirkill(cp);
	mirkill(cq);
}

void NumTools::task_powmod_crt
(const big& a, const big& n, const big& p, const big& q, const big& pq
, const big& pInv, big& res) {
	big cp = mirvar(0), cq = mirvar(0);
	sub_task_powmod(a, n, p, cp);// cp = a^n mod p
	sub_task_powmod(a, n, q, cq);// cq = a^n mod q
	subtract(cq, cp, cq);// cq = cq - cp
	multiply(cq, p, cq);
	// modmul by monty
	prepare_monty(pq);
	big nresL = mirvar(0);
	nres(cq, nresL);
	big nresR = mirvar(0);
	nres(pInv, nresR);
	big nresRes = mirvar(0);
	nres_modmult(nresL, nresR, nresRes);
	nres(cp, nresL);
	nres_modadd(nresL, nresRes, nresRes);
	redc(nresRes, res);
	mirkill(nresL);
	mirkill(nresR);
	mirkill(nresRes);
	mirkill(cp);
	mirkill(cq);
}
