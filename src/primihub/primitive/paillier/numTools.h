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


#ifndef __NUMTOOLS__
#define __NUMTOOLS__

#include "miracl.h"
#include "mirdef.h"

class NumTools {
public:
    static void modmul(const big& x, const big& y, const big& n, big& res);

    static void sub_task_powmod(const big& a, const big& n, const big& p, big& res);

    static void task_powmod_brick_crt(const big& n, const big& pInv, brick* bp, brick* bq
									, const big& p, const big& q, const big& pq, big& res);

	static void task_powmod_crt(const big& a, const big& n, const big& p, const big& q, const big& pq
                                    , const big& pInv, big& res);
};

#endif
