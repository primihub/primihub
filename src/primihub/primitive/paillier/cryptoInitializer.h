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


#ifndef __INITIALIZER__
#define __INITIALIZER__

extern "C"
{
#include "miracl.h"
#include "mirdef.h"
}
#include <ctime>
#include <mutex>

class CryptoInitializer {
private:
	miracl* mip;
	int MEMBASE;// for computing and storing
	int IOBASE;// for printing
	int VARSIZE;// the biggest size of "big,flash" var

private:
	
	CryptoInitializer(int varsize = 400, int membase = 256, int iobase = 10)
	: VARSIZE(varsize), MEMBASE(membase), IOBASE(iobase) {
		if (membase < 2 || membase > 256) {
			throw "the seconde parameter must greater than 2 and less than 256";
		}
		// default: mip=mirsys(400,256) for efficiency
		mip = mirsys(VARSIZE, MEMBASE);
		mip->IOBASE = this->IOBASE;
		irand((unsigned int)time(0));
	}

	~CryptoInitializer() {
		// mirexit: free mip
		mirexit();
	}

	static CryptoInitializer* instancePtr;
	static std::mutex mtx;

public:

	CryptoInitializer(CryptoInitializer&) = delete;

	CryptoInitializer&
		operator=(const CryptoInitializer&) = delete;

public:

	static void startUp(int varsize = 400, int membase = 256, int iobase = 10);

	static void close();

	static int getIOBASE();

	static void setIOBASE(int iobase);

};

#endif
