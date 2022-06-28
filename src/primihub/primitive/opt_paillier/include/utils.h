/**
  \file 		utils.h
  \author 	
  \copyright	ABY - A Framework for Efficient Mixed-protocol Secure Two-party Computation
			Copyright (C) 2019 ENCRYPTO Group, TU Darmstadt
			This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published
            by the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.
            ABY is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
            GNU Lesser General Public License for more details.
            You should have received a copy of the GNU Lesser General Public License
            along with this program. If not, see <http://www.gnu.org/licenses/>.
  \brief		utils
 */

#ifndef _UTILS_H__
#define _UTILS_H__

#include <cstdint>
#include <gmp.h>
#include <unistd.h>

#ifdef WIN32
#define SleepMiliSec(x)	Sleep(x)
#else
#define SleepMiliSec(x)			usleep((x)<<10)
#endif

#define two_pow(e) (((uint64_t) 1) << (e))

#define pad_to_power_of_two(e) ( ((uint64_t) 1) << (ceil_log2(e)) )

/*compute (a-b) mod (m+1) as: b > a ? (m) - (b-1) + a : a - b	*/
#define MOD_SUB(a, b, m) (( ((b) > (a))? (m) - ((b) -1 ) + a : a - b))

#define ceil_divide(x, y)			(( ((x) + (y)-1)/(y)))
#define bits_in_bytes(bits) (ceil_divide((bits), 8))
#define pad_to_multiple(x, y) 		( ceil_divide(x, y) * (y))

#define PadToRegisterSize(x) 		(PadToMultiple(x, OTEXT_BLOCK_SIZE_BITS))
#define PadToMultiple(x, y) 		( ceil_divide(x, y) * (y))

//TODO: this is bad, fix occurrences of ceil_log2 and replace by ceil_log2_min1 where log(1) = 1 is necessary. For all else use ceil_log2_real
uint32_t ceil_log2(int bits);

uint32_t ceil_log2_min1(int bits);

uint32_t ceil_log2_real(int bits);

uint32_t floor_log2(int bits);

/**
 * returns a 4-byte value from dev/random
 */
uint32_t aby_rand();

/**
 * returns a random mpz_t with bitlen len generated from dev/urandom
 */
void aby_prng(mpz_t rnd, mp_bitcnt_t len);

#endif // _UTILS_H__
