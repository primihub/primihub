/*
 Copyright 2022 PrimiHub

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


#include <stdio.h>
#include <time.h>
#include <chrono>
#include <iostream>
#include <random>

#include "paillier.h"

int main() {
	int secK = 1024, successCount = 0;
    miracl* mip = mirsys(4000, 256);
	mip->IOBASE = 10;
	// n = 2 * secK = 2048 => l = 448
	Paillier* p = new Paillier(secK, 448);

	// p->keyGen();
	// read key parameters from files
	p->keyRead();

	//	take some tests for encryption and decryption

	double encrypt_cost = 0.0, decrypt_cost = 0.0;
	const int round = 100;

	bool flag = true;

	std::cout << "========================================================" << std::endl;
	try {
		p->encryptionCrtPrepare();
		for (int i = 1; i <= round; ++i) {
			big plain = mirvar(0);
			bigrand(p->pk.N, plain);
			auto e_st = std::chrono::high_resolution_clock::now();
			big c = p->encryptionCrt(plain);
			auto e_ed = std::chrono::high_resolution_clock::now();
			encrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(e_ed - e_st).count();

			auto d_st = std::chrono::high_resolution_clock::now();
			big d = p->decryptionPaiCrt(c);
			auto d_ed = std::chrono::high_resolution_clock::now();
			decrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(d_ed - d_st).count();

			if (0 != mr_compare(d, plain)) {
				std::cout << "Error: result of encryption is error!" << std::endl;
				// std::cout << "Plaintext is : " << curPlain
				// 		<< ", Result of decryption is : " << d << std::endl;
				flag = false;
			}
			else {
				++successCount;
			}
			mirkill(d);
			mirkill(c);
			mirkill(plain);
		}
		p->encryptionCrtEnd();
	}
	catch(const char* e) {
		std::cout << e << std::endl;
	}

	if (flag)
	{
		std::cout << "Number of test cases is " << round
				<< ", Number of successful test cases is " << successCount << std:: endl;
		std::cout << "All correct!" << std::endl;
	}


	std::cout << "========================================================" << std::endl;

	encrypt_cost = 1.0 / round * encrypt_cost;
	decrypt_cost = 1.0 / round * decrypt_cost;

	std::cout << "The total encryption cost is " << encrypt_cost / 1000.0 << " ms." << std::endl;
	std::cout << "The total decryption cost is " << decrypt_cost / 1000.0 << " ms." << std::endl;

	std::cout << "========================================================" << std::endl;
	delete p;
	mirexit();
	return 0;
}
