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


#include "cryptoInitializer.h"

CryptoInitializer* CryptoInitializer::instancePtr = nullptr;
std::mutex CryptoInitializer::mtx;

void CryptoInitializer::startUp(int varsize, int membase, int iobase) {
	if (!instancePtr) {
		std::lock_guard<std::mutex> lock(mtx);
		if (!instancePtr) {
			instancePtr = new CryptoInitializer(varsize, membase, iobase);
		}
	}
}

int CryptoInitializer::getIOBASE() {
	int defaultIOBASE = 10;
	if (instancePtr) {
		defaultIOBASE = instancePtr->mip->IOBASE;
	}
	return defaultIOBASE;
}

void CryptoInitializer::setIOBASE(int iobase) {
	if (iobase < 2 || iobase > 256) {
		throw "the seconde parameter must greater than 2 and less than 256";
	}
	if (!instancePtr) {
		throw "Cryptographic system instance was destroyed";
	}
	instancePtr->mip->IOBASE = iobase;
}

void CryptoInitializer::close() {
	if (instancePtr) {
		delete instancePtr;
	}
	instancePtr = nullptr;
}
