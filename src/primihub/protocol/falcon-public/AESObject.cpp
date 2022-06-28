
#pragma once
#include "util/TedKrovetzAesNiWrapperC.h"
#include "util/block.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include "AESObject.h"

using namespace std;

namespace primihub
{
	namespace falcon
	{
		AESObject::AESObject(char *filename)
		{
			ifstream f(filename);
			string str{istreambuf_iterator<char>(f), istreambuf_iterator<char>()};
			f.close();
			int len = str.length();
			char common_aes_key[len + 1];
			memset(common_aes_key, '\0', len + 1);
			strcpy(common_aes_key, str.c_str());
			AES_set_encrypt_key((unsigned char *)common_aes_key, 256, &aes_key);
		}

		__m128i AESObject::newRandomNumber()
		{
			rCounter++;
			if (rCounter % RANDOM_COMPUTE == 0) // generate more random seeds
			{
				for (int i = 0; i < RANDOM_COMPUTE; i++)
#ifdef ENABLE_SSE
					tempSecComp[i] = _mm_set1_epi32(rCounter + i); // not exactly counter mode - (rcounter+i,rcouter+i,rcounter+i,rcounter+i)
#else
					TODO("Implement it.");
#endif
				AES_ecb_encrypt_chunk_in_out(tempSecComp, pseudoRandomString, RANDOM_COMPUTE, &aes_key);
			}
			return pseudoRandomString[rCounter % RANDOM_COMPUTE];
		}

		myType AESObject::get64Bits()
		{
			myType ret;

			if (random64BitCounter == 0)
				random64BitNumber = newRandomNumber();

			int x = random64BitCounter % 2;
			uint64_t *temp = (uint64_t *)&random64BitNumber;

			switch (x)
			{
			case 0:
				ret = (myType)temp[1];
				break;
			case 1:
				ret = (myType)temp[0];
				break;
			}

			random64BitCounter++;
			if (random64BitCounter == 2)
				random64BitCounter = 0;

			return ret;
		}

		smallType AESObject::get8Bits()
		{
			smallType ret;

			if (random8BitCounter == 0)
				random8BitNumber = newRandomNumber();

			uint8_t *temp = (uint8_t *)&random8BitNumber;
			ret = (smallType)temp[random8BitCounter];

			random8BitCounter++;
			if (random8BitCounter == 16)
				random8BitCounter = 0;

			return ret;
		}

		smallType AESObject::randModPrime()
		{
			smallType ret;

			do
			{
				ret = get8Bits();
			} while (ret >= BOUNDARY);

			return (ret % PRIME_NUMBER);
		}

		smallType AESObject::randNonZeroModPrime()
		{
			smallType ret;
			do
			{
				ret = randModPrime();
			} while (ret == 0);

			return ret;
		}

		smallType AESObject::AES_random(int i)
		{
			smallType ret;
			do
			{
				ret = get8Bits();
			} while (ret >= ((256 / i) * i));

			return (ret % i);
		}
	} // namespace primihub{
}