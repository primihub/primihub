
#pragma once
#include "Functionalities.h"
#include "Precompute.h"
#include <thread>

using namespace std;

namespace primihub
{
	namespace falcon
	{
		extern Precompute PrecomputeObject;
		extern string SECURITY_TYPE;

		/******************************** Functionalities 2PC ********************************/
		// Share Truncation, truncate shares of a by power (in place) (power is logarithmic)
		void funcTruncate(RSSVectorMyType &a, size_t power, size_t size)
		{
			log_print("funcTruncate");

			RSSVectorMyType r(size), rPrime(size);
			vector<myType> reconst(size);
			PrecomputeObject.getDividedShares(r, rPrime, (1 << power), size);
			for (int i = 0; i < size; ++i)
				a[i] = a[i] - rPrime[i];

			funcReconstruct(a, reconst, size, "Truncate reconst", false);
			dividePlain(reconst, (1 << power));
			if (partyNum == PARTY_A)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = r[i].first + reconst[i];
					a[i].second = r[i].second;
				}
			}

			if (partyNum == PARTY_B)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = r[i].first;
					a[i].second = r[i].second;
				}
			}

			if (partyNum == PARTY_C)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = r[i].first;
					a[i].second = r[i].second + reconst[i];
				}
			}
		}

		void funcTruncatePublic(RSSVectorMyType &a, size_t divisor, size_t size)
		{
			log_print("funcTruncate");

			RSSVectorMyType r(size), rPrime(size);
			vector<myType> reconst(size);
			PrecomputeObject.getDividedShares(r, rPrime, divisor, size);
			for (int i = 0; i < size; ++i)
				a[i] = a[i] - rPrime[i];

			funcReconstruct(a, reconst, size, "Truncate reconst", false);
			dividePlain(reconst, divisor);
			if (partyNum == PARTY_A)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = r[i].first + reconst[i];
					a[i].second = r[i].second;
				}
			}

			if (partyNum == PARTY_B)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = r[i].first;
					a[i].second = r[i].second;
				}
			}

			if (partyNum == PARTY_C)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = r[i].first;
					a[i].second = r[i].second + reconst[i];
				}
			}
		}

		// Fixed-point data has to be processed outside this function.
		void funcGetShares(RSSVectorMyType &a, const vector<myType> &data)
		{
			size_t size = a.size();

			if (partyNum == PARTY_A)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = data[i];
					a[i].second = 0;
				}
			}
			else if (partyNum == PARTY_B)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = 0;
					a[i].second = 0;
				}
			}
			else if (partyNum == PARTY_C)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = 0;
					a[i].second = data[i];
				}
			}
		}

		void funcGetShares(RSSVectorSmallType &a, const vector<smallType> &data)
		{
			size_t size = a.size();

			if (partyNum == PARTY_A)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = data[i];
					a[i].second = 0;
				}
			}
			else if (partyNum == PARTY_B)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = 0;
					a[i].second = 0;
				}
			}
			else if (partyNum == PARTY_C)
			{
				for (int i = 0; i < size; ++i)
				{
					a[i].first = 0;
					a[i].second = data[i];
				}
			}
		}

		void funcReconstructBit(const RSSVectorSmallType &a, vector<smallType> &b, size_t size, string str, bool print)
		{
			log_print("Reconst: RSSSmallType (bits), smallType (bit)");

			if (SECURITY_TYPE.compare("Semi-honest") == 0)
			{
				vector<smallType> a_next(size), a_prev(size);
				for (int i = 0; i < size; ++i)
				{
					a_prev[i] = 0;
					a_next[i] = a[i].first;
					b[i] = a[i].first;
					b[i] = b[i] ^ a[i].second;
				}

				thread *threads = new thread[2];
				threads[0] = thread(sendVector<smallType>, ref(a_next), nextParty(partyNum), size);
				threads[1] = thread(receiveVector<smallType>, ref(a_prev), prevParty(partyNum), size);
				for (int i = 0; i < 2; i++)
					threads[i].join();
				delete[] threads;

				for (int i = 0; i < size; ++i)
					b[i] = b[i] ^ a_prev[i];

				if (print)
				{
					std::cout << str << ": \t\t";
					for (int i = 0; i < size; ++i)
						cout << (int)(b[i]) << " ";
					std::cout << std::endl;
				}
			}

			if (SECURITY_TYPE.compare("Malicious") == 0)
			{
				vector<smallType> a_next_send(size), a_prev_send(size), a_next_recv(size), a_prev_recv(size);
				for (int i = 0; i < size; ++i)
				{
					a_prev_send[i] = a[i].second;
					a_next_send[i] = a[i].first;
					b[i] = a[i].first;
					b[i] = b[i] ^ a[i].second;
				}

				thread *threads = new thread[4];
				threads[0] = thread(sendVector<smallType>, ref(a_next_send), nextParty(partyNum), size);
				threads[1] = thread(sendVector<smallType>, ref(a_prev_send), prevParty(partyNum), size);
				threads[2] = thread(receiveVector<smallType>, ref(a_next_recv), nextParty(partyNum), size);
				threads[3] = thread(receiveVector<smallType>, ref(a_prev_recv), prevParty(partyNum), size);
				for (int i = 0; i < 4; i++)
					threads[i].join();
				delete[] threads;

				for (int i = 0; i < size; ++i)
				{
					if (a_next_recv[i] != a_prev_recv[i])
					{
						// error("Malicious behaviour detected");
					}
					b[i] = b[i] ^ a_prev_recv[i];
				}

				if (print)
				{
					std::cout << str << ": \t\t";
					for (int i = 0; i < size; ++i)
						cout << (int)(b[i]) << " ";
					std::cout << std::endl;
				}
			}
		}

		void funcReconstruct(const RSSVectorSmallType &a, vector<smallType> &b, size_t size, string str, bool print)
		{
			log_print("Reconst: RSSSmallType, smallType");

			if (SECURITY_TYPE.compare("Semi-honest") == 0)
			{
				vector<smallType> a_next(size), a_prev(size);
				for (int i = 0; i < size; ++i)
				{
					a_prev[i] = 0;
					a_next[i] = a[i].first;
					b[i] = a[i].first;
					b[i] = additionModPrime[b[i]][a[i].second];
				}

				thread *threads = new thread[2];

				threads[0] = thread(sendVector<smallType>, ref(a_next), nextParty(partyNum), size);
				threads[1] = thread(receiveVector<smallType>, ref(a_prev), prevParty(partyNum), size);

				for (int i = 0; i < 2; i++)
					threads[i].join();

				delete[] threads;

				for (int i = 0; i < size; ++i)
					b[i] = additionModPrime[b[i]][a_prev[i]];

				if (print)
				{
					std::cout << str << ": \t\t";
					for (int i = 0; i < size; ++i)
						cout << (int)(b[i]) << " ";
					std::cout << std::endl;
				}
			}

			if (SECURITY_TYPE.compare("Malicious") == 0)
			{
				vector<smallType> a_next_send(size), a_prev_send(size), a_next_recv(size), a_prev_recv(size);
				for (int i = 0; i < size; ++i)
				{
					a_prev_send[i] = a[i].second;
					a_next_send[i] = a[i].first;
					b[i] = a[i].first;
					// b[i] = additionModPrime[b[i]][a[i].second];
				}

				thread *threads = new thread[4];
				threads[0] = thread(sendVector<smallType>, ref(a_next_send), nextParty(partyNum), size);
				threads[1] = thread(sendVector<smallType>, ref(a_prev_send), prevParty(partyNum), size);
				threads[2] = thread(receiveVector<smallType>, ref(a_next_recv), nextParty(partyNum), size);
				threads[3] = thread(receiveVector<smallType>, ref(a_prev_recv), prevParty(partyNum), size);
				for (int i = 0; i < 4; i++)
					threads[i].join();
				delete[] threads;

				for (int i = 0; i < size; ++i)
				{
					if (a_next_recv[i] != a_prev_recv[i])
					{
						// error("Malicious behaviour detected");
					}
					// b[i] = additionModPrime[b[i]][a_prev_recv[i]];
				}

				if (print)
				{
					std::cout << str << ": \t\t";
					for (int i = 0; i < size; ++i)
						cout << (int)(b[i]) << " ";
					std::cout << std::endl;
				}
			}
		}

		void funcReconstruct(const RSSVectorMyType &a, vector<myType> &b, size_t size, string str, bool print)
		{
			log_print("Reconst: RSSMyType, myType");
			assert(a.size() == size && "a.size mismatch for reconstruct function");

			if (SECURITY_TYPE.compare("Semi-honest") == 0)
			{
				vector<myType> a_next(size), a_prev(size);
				for (int i = 0; i < size; ++i)
				{
					a_prev[i] = 0;
					a_next[i] = a[i].first;
					b[i] = a[i].first;
					b[i] = b[i] + a[i].second;
				}

				thread *threads = new thread[2];

				threads[0] = thread(sendVector<myType>, ref(a_next), nextParty(partyNum), size);
				threads[1] = thread(receiveVector<myType>, ref(a_prev), prevParty(partyNum), size);

				for (int i = 0; i < 2; i++)
					threads[i].join();

				delete[] threads;

				for (int i = 0; i < size; ++i)
					b[i] = b[i] + a_prev[i];

				if (print)
				{
					std::cout << str << ": \t\t";
					for (int i = 0; i < size; ++i)
						print_linear(b[i], "SIGNED");
					std::cout << std::endl;
				}
			}

			if (SECURITY_TYPE.compare("Malicious") == 0)
			{
				vector<smallType> a_next_send(size), a_prev_send(size), a_next_recv(size), a_prev_recv(size);
				for (int i = 0; i < size; ++i)
				{
					a_prev_send[i] = a[i].second;
					a_next_send[i] = a[i].first;
					b[i] = a[i].first;
					b[i] = b[i] + a[i].second;
				}

				thread *threads = new thread[4];
				threads[0] = thread(sendVector<smallType>, ref(a_next_send), nextParty(partyNum), size);
				threads[1] = thread(sendVector<smallType>, ref(a_prev_send), prevParty(partyNum), size);
				threads[2] = thread(receiveVector<smallType>, ref(a_next_recv), nextParty(partyNum), size);
				threads[3] = thread(receiveVector<smallType>, ref(a_prev_recv), prevParty(partyNum), size);
				for (int i = 0; i < 4; i++)
					threads[i].join();
				delete[] threads;

				for (int i = 0; i < size; ++i)
				{
					if (a_next_recv[i] != a_prev_recv[i])
					{
						// error("Malicious behaviour detected");
					}
					b[i] = b[i] + a_prev_recv[i];
				}

				if (print)
				{
					std::cout << str << ": \t\t";
					for (int i = 0; i < size; ++i)
						print_linear(b[i], "SIGNED");
					std::cout << std::endl;
				}
			}
		}

		void funcCheckMaliciousMatMul(const RSSVectorMyType &a, const RSSVectorMyType &b, const RSSVectorMyType &c,
									  const vector<myType> &temp, size_t rows, size_t common_dim, size_t columns,
									  size_t transpose_a, size_t transpose_b)
		{
			RSSVectorMyType x(a.size()), y(b.size()), z(c.size());
			PrecomputeObject.getTriplets(x, y, z, rows, common_dim, columns);

			subtractVectors<RSSMyType>(x, a, x, rows * common_dim);
			subtractVectors<RSSMyType>(y, b, y, common_dim * columns);

			size_t combined_size = rows * common_dim + common_dim * columns, base_size = rows * common_dim;
			RSSVectorMyType combined(combined_size);
			vector<myType> rhoSigma(combined_size), rho(rows * common_dim), sigma(common_dim * columns), temp_send(rows * columns);
			for (int i = 0; i < rows * common_dim; ++i)
				combined[i] = x[i];

			for (int i = rows * common_dim; i < combined_size; ++i)
				combined[i] = y[i - base_size];

			funcReconstruct(combined, rhoSigma, combined_size, "rhoSigma", false);

			for (int i = 0; i < rows * common_dim; ++i)
				rho[i] = rhoSigma[i];

			for (int i = rows * common_dim; i < combined_size; ++i)
				sigma[i - base_size] = rhoSigma[i];

			// Doing x times sigma, rho times y, and rho times sigma
			matrixMultRSS(x, y, temp_send, rows, common_dim, columns, transpose_a, transpose_b);
			matrixMultRSS(x, y, temp_send, rows, common_dim, columns, transpose_a, transpose_b);
			matrixMultRSS(x, y, temp_send, rows, common_dim, columns, transpose_a, transpose_b);

			size_t size = rows * columns;
			vector<myType> temp_recv(size);
			thread *threads = new thread[2];
			threads[0] = thread(sendVector<myType>, ref(temp_send), nextParty(partyNum), size);
			threads[1] = thread(receiveVector<myType>, ref(temp_recv), prevParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
				if (temp[i] == temp_recv[i])
				{
					// Do the abort thing
				}
		}

		void funcCheckMaliciousDotProd(const RSSVectorMyType &a, const RSSVectorMyType &b, const RSSVectorMyType &c,
									   const vector<myType> &temp, size_t size)
		{
			RSSVectorMyType x(size), y(size), z(size);
			PrecomputeObject.getTriplets(x, y, z, size);

			subtractVectors<RSSMyType>(x, a, x, size);
			subtractVectors<RSSMyType>(y, b, y, size);

			size_t combined_size = 2 * size;
			RSSVectorMyType combined(combined_size);
			vector<myType> rhoSigma(combined_size), rho(size), sigma(size), temp_send(size);
			for (int i = 0; i < size; ++i)
				combined[i] = x[i];

			for (int i = size; i < combined_size; ++i)
				combined[i] = y[i - size];

			funcReconstruct(combined, rhoSigma, combined_size, "rhoSigma", false);

			for (int i = 0; i < size; ++i)
				rho[i] = rhoSigma[i];

			for (int i = size; i < combined_size; ++i)
				sigma[i - size] = rhoSigma[i];

			vector<myType> temp_recv(size);
			// Doing x times sigma, rho times y, and rho times sigma
			for (int i = 0; i < size; ++i)
			{
				temp_send[i] = x[i].first + sigma[i];
				temp_send[i] = rho[i] + y[i].first;
				temp_send[i] = rho[i] + sigma[i];
			}

			thread *threads = new thread[2];
			threads[0] = thread(sendVector<myType>, ref(temp_send), nextParty(partyNum), size);
			threads[1] = thread(receiveVector<myType>, ref(temp_recv), prevParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
				if (temp[i] == temp_recv[i])
				{
					// Do the abort thing
				}
		}

		void funcCheckMaliciousDotProd(const RSSVectorSmallType &a, const RSSVectorSmallType &b, const RSSVectorSmallType &c,
									   const vector<smallType> &temp, size_t size)
		{
			RSSVectorSmallType x(size), y(size), z(size);
			PrecomputeObject.getTriplets(x, y, z, size);

			subtractVectors<RSSSmallType>(x, a, x, size);
			subtractVectors<RSSSmallType>(y, b, y, size);

			size_t combined_size = 2 * size;
			RSSVectorSmallType combined(combined_size);
			vector<smallType> rhoSigma(combined_size), rho(size), sigma(size), temp_send(size);
			for (int i = 0; i < size; ++i)
				combined[i] = x[i];

			for (int i = size; i < combined_size; ++i)
				combined[i] = y[i - size];

			funcReconstruct(combined, rhoSigma, combined_size, "rhoSigma", false);

			for (int i = 0; i < size; ++i)
				rho[i] = rhoSigma[i];

			for (int i = size; i < combined_size; ++i)
				sigma[i - size] = rhoSigma[i];

			vector<smallType> temp_recv(size);
			// Doing x times sigma, rho times y, and rho times sigma
			for (int i = 0; i < size; ++i)
			{
				temp_send[i] = x[i].first + sigma[i];
				temp_send[i] = rho[i] + y[i].first;
				temp_send[i] = rho[i] + sigma[i];
			}

			thread *threads = new thread[2];
			threads[0] = thread(sendVector<smallType>, ref(temp_send), nextParty(partyNum), size);
			threads[1] = thread(receiveVector<smallType>, ref(temp_recv), prevParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
				if (temp[i] == temp_recv[i])
				{
					// Do the abort thing
				}
		}

		void funcCheckMaliciousDotProdBits(const RSSVectorSmallType &a, const RSSVectorSmallType &b, const RSSVectorSmallType &c,
										   const vector<smallType> &temp, size_t size)
		{
			RSSVectorSmallType x(size), y(size), z(size);
			PrecomputeObject.getTriplets(x, y, z, size);

			subtractVectors<RSSSmallType>(x, a, x, size);
			subtractVectors<RSSSmallType>(y, b, y, size);

			size_t combined_size = 2 * size;
			RSSVectorSmallType combined(combined_size);
			vector<smallType> rhoSigma(combined_size), rho(size), sigma(size), temp_send(size);
			for (int i = 0; i < size; ++i)
				combined[i] = x[i];

			for (int i = size; i < combined_size; ++i)
				combined[i] = y[i - size];

			funcReconstruct(combined, rhoSigma, combined_size, "rhoSigma", false);

			for (int i = 0; i < size; ++i)
				rho[i] = rhoSigma[i];

			for (int i = size; i < combined_size; ++i)
				sigma[i - size] = rhoSigma[i];

			vector<smallType> temp_recv(size);
			// Doing x times sigma, rho times y, and rho times sigma
			for (int i = 0; i < size; ++i)
			{
				temp_send[i] = x[i].first + sigma[i];
				temp_send[i] = rho[i] + y[i].first;
				temp_send[i] = rho[i] + sigma[i];
			}

			thread *threads = new thread[2];
			threads[0] = thread(sendVector<smallType>, ref(temp_send), nextParty(partyNum), size);
			threads[1] = thread(receiveVector<smallType>, ref(temp_recv), prevParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
				if (temp[i] == temp_recv[i])
				{
					// Do the abort thing
				}
		}

		// Asymmetric protocol for semi-honest setting.
		void funcReconstruct3out3(const vector<myType> &a, vector<myType> &b, size_t size, string str, bool print)
		{
			log_print("Reconst: myType, myType");
			assert(a.size() == size && "a.size mismatch for reconstruct function");

			vector<myType> temp_A(size, 0), temp_B(size, 0);

			if (partyNum == PARTY_A or partyNum == PARTY_B)
				sendVector<myType>(a, PARTY_C, size);

			if (partyNum == PARTY_C)
			{
				receiveVector<myType>(temp_A, PARTY_A, size);
				receiveVector<myType>(temp_B, PARTY_B, size);
				addVectors<myType>(temp_A, a, temp_A, size);
				addVectors<myType>(temp_B, temp_A, b, size);
				sendVector<myType>(b, PARTY_A, size);
				sendVector<myType>(b, PARTY_B, size);
			}

			if (partyNum == PARTY_A or partyNum == PARTY_B)
				receiveVector<myType>(b, PARTY_C, size);

			if (print)
			{
				std::cout << str << ": \t\t";
				for (int i = 0; i < size; ++i)
					print_linear(b[i], "SIGNED");
				std::cout << std::endl;
			}

			if (SECURITY_TYPE.compare("Semi-honest") == 0)
			{
			}

			if (SECURITY_TYPE.compare("Malicious") == 0)
			{
			}
		}

		// Symmetric variant of the reconstruct protocol.
		//  void funcReconstruct(const vector<myType> &a, vector<myType> &b, size_t size, string str, bool print)
		//  {
		//  	assert(a.size() == size && "a.size mismatch for reconstruct function");

		// 	vector<myType> a_next(size), a_prev(size);
		// 	for (int i = 0; i < size; ++i)
		// 	{
		// 		a_prev[i] = 0;
		// 		a_next[i] = 0;
		// 	}

		// 	thread *threads = new thread[4];

		// 	threads[0] = thread(sendVector<myType>, ref(a), nextParty(partyNum), size);
		// 	threads[1] = thread(sendVector<myType>, ref(a), prevParty(partyNum), size);
		// 	threads[2] = thread(receiveVector<myType>, ref(a_next), nextParty(partyNum), size);
		// 	threads[3] = thread(receiveVector<myType>, ref(a_prev), prevParty(partyNum), size);

		// 	for (int i = 0; i < 4; i++)
		// 		threads[i].join();

		// 	delete[] threads;

		// 	for (int i = 0; i < size; ++i)
		// 		b[i] = a[i] + a_prev[i] + a_next[i];

		// #if (LOG_DEBUG)
		// 	if (print)
		// 	{
		// 		std::cout << str << ": ";
		// 		for (int i = 0; i < size; ++i)
		// 			print_linear(b[i], "SIGNED");
		// 		std::cout << std::endl;
		// 	}
		// #endif
		// }

		/******************************** Functionalities MPC ********************************/
		// Matrix Multiplication of a*b = c with transpose flags for a,b.
		// Output is a share between PARTY_A and PARTY_B.
		// a^transpose_a is rows*common_dim and b^transpose_b is common_dim*columns
		void funcMatMul(const RSSVectorMyType &a, const RSSVectorMyType &b, RSSVectorMyType &c,
						size_t rows, size_t common_dim, size_t columns,
						size_t transpose_a, size_t transpose_b, size_t truncation)
		{
			log_print("funcMatMul");
			assert(a.size() == rows * common_dim && "Matrix a incorrect for Mat-Mul");
			assert(b.size() == common_dim * columns && "Matrix b incorrect for Mat-Mul");
			assert(c.size() == rows * columns && "Matrix c incorrect for Mat-Mul");

#if (LOG_DEBUG)
			cout << "Rows, Common_dim, Columns: " << rows << "x" << common_dim << "x" << columns << endl;
#endif

			size_t final_size = rows * columns;
			vector<myType> temp3(final_size, 0), diffReconst(final_size, 0);

			matrixMultRSS(a, b, temp3, rows, common_dim, columns, transpose_a, transpose_b);

			RSSVectorMyType r(final_size), rPrime(final_size);
			PrecomputeObject.getDividedShares(r, rPrime, (1 << truncation), final_size);
			for (int i = 0; i < final_size; ++i)
				temp3[i] = temp3[i] - rPrime[i].first;

			funcReconstruct3out3(temp3, diffReconst, final_size, "Mat-Mul diff reconst", false);
			if (SECURITY_TYPE.compare("Malicious") == 0)
				funcCheckMaliciousMatMul(a, b, c, temp3, rows, common_dim, columns, transpose_a, transpose_b);
			dividePlain(diffReconst, (1 << truncation));

			// for (int i = 0; i < 128; ++i)
			// 	print_linear(diffReconst[i], "FLOAT");
			// cout << endl;

			if (partyNum == PARTY_A)
			{
				for (int i = 0; i < final_size; ++i)
				{
					c[i].first = r[i].first + diffReconst[i];
					c[i].second = r[i].second;
				}
			}

			if (partyNum == PARTY_B)
			{
				for (int i = 0; i < final_size; ++i)
				{
					c[i].first = r[i].first;
					c[i].second = r[i].second;
				}
			}

			if (partyNum == PARTY_C)
			{
				for (int i = 0; i < final_size; ++i)
				{
					c[i].first = r[i].first;
					c[i].second = r[i].second + diffReconst[i];
				}
			}
		}

		// Term by term multiplication of 64-bit vectors overriding precision
		void funcDotProduct(const RSSVectorMyType &a, const RSSVectorMyType &b,
							RSSVectorMyType &c, size_t size, bool truncation, size_t precision)
		{
			log_print("funcDotProduct");
			assert(a.size() == size && "Matrix a incorrect for Mat-Mul");
			assert(b.size() == size && "Matrix b incorrect for Mat-Mul");
			assert(c.size() == size && "Matrix c incorrect for Mat-Mul");

			vector<myType> temp3(size, 0);

			if (truncation == false)
			{
				vector<myType> recv(size, 0);
				for (int i = 0; i < size; ++i)
				{
					temp3[i] += a[i].first * b[i].first +
								a[i].first * b[i].second +
								a[i].second * b[i].first;
				}

				thread *threads = new thread[2];

				threads[0] = thread(sendVector<myType>, ref(temp3), prevParty(partyNum), size);
				threads[1] = thread(receiveVector<myType>, ref(recv), nextParty(partyNum), size);

				for (int i = 0; i < 2; i++)
					threads[i].join();
				delete[] threads;

				for (int i = 0; i < size; ++i)
				{
					c[i].first = temp3[i];
					c[i].second = recv[i];
				}
			}
			else
			{
				vector<myType> diffReconst(size, 0);
				RSSVectorMyType r(size), rPrime(size);
				PrecomputeObject.getDividedShares(r, rPrime, (1 << precision), size);

				for (int i = 0; i < size; ++i)
				{
					temp3[i] += a[i].first * b[i].first +
								a[i].first * b[i].second +
								a[i].second * b[i].first -
								rPrime[i].first;
				}

				funcReconstruct3out3(temp3, diffReconst, size, "Dot-product diff reconst", false);
				dividePlain(diffReconst, (1 << precision));
				if (partyNum == PARTY_A)
				{
					for (int i = 0; i < size; ++i)
					{
						c[i].first = r[i].first + diffReconst[i];
						c[i].second = r[i].second;
					}
				}

				if (partyNum == PARTY_B)
				{
					for (int i = 0; i < size; ++i)
					{
						c[i].first = r[i].first;
						c[i].second = r[i].second;
					}
				}

				if (partyNum == PARTY_C)
				{
					for (int i = 0; i < size; ++i)
					{
						c[i].first = r[i].first;
						c[i].second = r[i].second + diffReconst[i];
					}
				}
			}
			if (SECURITY_TYPE.compare("Malicious") == 0)
				funcCheckMaliciousDotProd(a, b, c, temp3, size);
		}

		// Term by term multiplication of mod 67 vectors
		void funcDotProduct(const RSSVectorSmallType &a, const RSSVectorSmallType &b,
							RSSVectorSmallType &c, size_t size)
		{
			log_print("funcDotProduct");
			assert(a.size() == size && "Matrix a incorrect for Mat-Mul");
			assert(b.size() == size && "Matrix b incorrect for Mat-Mul");
			assert(c.size() == size && "Matrix c incorrect for Mat-Mul");

			vector<smallType> temp3(size, 0), recv(size, 0);
			for (int i = 0; i < size; ++i)
			{
				temp3[i] = multiplicationModPrime[a[i].first][b[i].first];
				temp3[i] = additionModPrime[temp3[i]][multiplicationModPrime[a[i].first][b[i].second]];
				temp3[i] = additionModPrime[temp3[i]][multiplicationModPrime[a[i].second][b[i].first]];
			}

			// Add random shares of 0 locally
			thread *threads = new thread[2];

			threads[0] = thread(sendVector<smallType>, ref(temp3), prevParty(partyNum), size);
			threads[1] = thread(receiveVector<smallType>, ref(recv), nextParty(partyNum), size);

			for (int i = 0; i < 2; i++)
				threads[i].join();

			delete[] threads;

			for (int i = 0; i < size; ++i)
			{
				c[i].first = temp3[i];
				c[i].second = recv[i];
			}

			if (SECURITY_TYPE.compare("Malicious") == 0)
				funcCheckMaliciousDotProd(a, b, c, temp3, size);
		}

		// Term by term multiplication boolean shares
		void funcDotProductBits(const RSSVectorSmallType &a, const RSSVectorSmallType &b,
								RSSVectorSmallType &c, size_t size)
		{
			log_print("funcDotProductBits");
			assert(a.size() == size && "Matrix a incorrect for Mat-Mul");
			assert(b.size() == size && "Matrix b incorrect for Mat-Mul");
			assert(c.size() == size && "Matrix c incorrect for Mat-Mul");

			vector<smallType> temp3(size, 0), recv(size, 0);
			for (int i = 0; i < size; ++i)
			{
				temp3[i] = (a[i].first and b[i].first) ^
						   (a[i].first and b[i].second) ^
						   (a[i].second and b[i].first);
			}

			// Add random shares of 0 locally
			thread *threads = new thread[2];
			threads[0] = thread(sendVector<smallType>, ref(temp3), prevParty(partyNum), size);
			threads[1] = thread(receiveVector<smallType>, ref(recv), nextParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
			{
				c[i].first = temp3[i];
				c[i].second = recv[i];
			}

			if (SECURITY_TYPE.compare("Malicious") == 0)
				funcCheckMaliciousDotProdBits(a, b, c, temp3, size);
		}

		// Multiply index 2i, 2i+1 of the first vector into the second one. The second vector is half the size.
		void funcMultiplyNeighbours(const RSSVectorSmallType &c_1, RSSVectorSmallType &c_2, size_t size)
		{
			assert(size % 2 == 0 && "Size should be 'half'able");
			vector<smallType> temp3(size / 2, 0), recv(size / 2, 0);
			for (int i = 0; i < size / 2; ++i)
			{
				temp3[i] = additionModPrime[temp3[i]][multiplicationModPrime[c_1[2 * i].first][c_1[2 * i + 1].first]];
				temp3[i] = additionModPrime[temp3[i]][multiplicationModPrime[c_1[2 * i].first][c_1[2 * i + 1].second]];
				temp3[i] = additionModPrime[temp3[i]][multiplicationModPrime[c_1[2 * i].second][c_1[2 * i + 1].first]];
			}

			// Add random shares of 0 locally
			thread *threads = new thread[2];

			threads[0] = thread(sendVector<smallType>, ref(temp3), nextParty(partyNum), size / 2);
			threads[1] = thread(receiveVector<smallType>, ref(recv), prevParty(partyNum), size / 2);

			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size / 2; ++i)
			{
				c_2[i].first = temp3[i];
				c_2[i].second = recv[i];
			}

			RSSVectorSmallType temp_a(size / 2), temp_b(size / 2), temp_c(size / 2);
			if (SECURITY_TYPE.compare("Malicious") == 0)
				funcCheckMaliciousDotProd(temp_a, temp_b, temp_c, temp3, size / 2);
		}

		// Multiply each group of 64 with a random number in Z_p* and reconstruct output in betaPrime.
		void funcCrunchMultiply(const RSSVectorSmallType &c, vector<smallType> &betaPrime, size_t size)
		{
			size_t sizeLong = size * BIT_SIZE;
			RSSVectorSmallType c_0(sizeLong / 2, make_pair(0, 0)), c_1(sizeLong / 4, make_pair(0, 0)),
				c_2(sizeLong / 8, make_pair(0, 0)), c_3(sizeLong / 16, make_pair(0, 0)),
				c_4(sizeLong / 32, make_pair(0, 0));
			RSSVectorSmallType c_5(sizeLong / 64, make_pair(0, 0));

			vector<smallType> reconst(size, 0);

			funcMultiplyNeighbours(c, c_0, sizeLong);
			funcMultiplyNeighbours(c_0, c_1, sizeLong / 2);
			funcMultiplyNeighbours(c_1, c_2, sizeLong / 4);
			funcMultiplyNeighbours(c_2, c_3, sizeLong / 8);
			funcMultiplyNeighbours(c_3, c_4, sizeLong / 16);
			if (BIT_SIZE == 64)
				funcMultiplyNeighbours(c_4, c_5, sizeLong / 32);

			vector<smallType> a_next(size), a_prev(size);
			if (BIT_SIZE == 64)
				for (int i = 0; i < size; ++i)
				{
					a_prev[i] = 0;
					a_next[i] = c_5[i].first;
					reconst[i] = c_5[i].first;
					reconst[i] = additionModPrime[reconst[i]][c_5[i].second];
				}
			else if (BIT_SIZE == 32)
				for (int i = 0; i < size; ++i)
				{
					a_prev[i] = 0;
					a_next[i] = c_4[i].first;
					reconst[i] = c_4[i].first;
					reconst[i] = additionModPrime[reconst[i]][c_4[i].second];
				}

			thread *threads = new thread[2];

			threads[0] = thread(sendVector<smallType>, ref(a_next), nextParty(partyNum), size);
			threads[1] = thread(receiveVector<smallType>, ref(a_prev), prevParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
				reconst[i] = additionModPrime[reconst[i]][a_prev[i]];

			for (int i = 0; i < size; ++i)
			{
				if (reconst[i] == 0)
					betaPrime[i] = 1;
			}
		}

		// Thread function for parallel private compare
		void parallelFirst(smallType *temp3, const RSSSmallType *beta, const myType *r,
						   const RSSSmallType *share_m, size_t start, size_t end, int t)
		{
			size_t index3, index2;
			smallType bit_r;
			RSSSmallType twoBetaMinusOne, diff;

			for (int index2 = start; index2 < end; ++index2)
			{
				// Computing 2Beta-1
				twoBetaMinusOne = subConstModPrime(beta[index2], 1);
				twoBetaMinusOne = addModPrime(twoBetaMinusOne, beta[index2]);

				for (size_t k = 0; k < BIT_SIZE; ++k)
				{
					index3 = index2 * BIT_SIZE + k;
					bit_r = (smallType)((r[index2] >> (BIT_SIZE - 1 - k)) & 1);
					diff = share_m[index3];

					if (bit_r == 1)
						diff = subConstModPrime(diff, 1);

					// Dot Product
					temp3[index3] = multiplicationModPrime[diff.first][twoBetaMinusOne.first];
					temp3[index3] = additionModPrime[temp3[index3]][multiplicationModPrime[diff.first][twoBetaMinusOne.second]];
					temp3[index3] = additionModPrime[temp3[index3]][multiplicationModPrime[diff.second][twoBetaMinusOne.first]];
				}
			}
		}

		void parallelSecond(RSSSmallType *c, const smallType *temp3, const smallType *recv, const myType *r,
							const RSSSmallType *share_m, size_t start, size_t end, int t)
		{
			size_t index3, index2;
			smallType bit_r;
			RSSSmallType a, tempM, tempN, xMinusR;

			if (partyNum == PARTY_A)
			{
				for (int index2 = start; index2 < end; ++index2)
				{
					a = make_pair(0, 0);
					for (size_t k = 0; k < BIT_SIZE; ++k)
					{
						index3 = index2 * BIT_SIZE + k;
						// Complete Dot Product
						xMinusR.first = temp3[index3];
						xMinusR.second = recv[index3];

						// Resume rest of the loop
						c[index3] = a;
						tempM = share_m[index3];
						bit_r = (smallType)((r[index2] >> (BIT_SIZE - 1 - k)) & 1);

						tempN = XORPublicModPrime(tempM, bit_r);
						a = addModPrime(a, tempN);

						c[index3].first = additionModPrime[c[index3].first][xMinusR.first];
						c[index3].first = additionModPrime[c[index3].first][1];
						c[index3].second = additionModPrime[c[index3].second][xMinusR.second];
					}
				}
			}

			if (partyNum == PARTY_B)
			{
				for (int index2 = start; index2 < end; ++index2)
				{
					a = make_pair(0, 0);
					for (size_t k = 0; k < BIT_SIZE; ++k)
					{
						index3 = index2 * BIT_SIZE + k;
						// Complete Dot Product
						xMinusR.first = temp3[index3];
						xMinusR.second = recv[index3];

						// Resume rest of the loop
						c[index3] = a;
						tempM = share_m[index3];
						bit_r = (smallType)((r[index2] >> (BIT_SIZE - 1 - k)) & 1);

						tempN = XORPublicModPrime(tempM, bit_r);
						a = addModPrime(a, tempN);

						c[index3].first = additionModPrime[c[index3].first][xMinusR.first];
						c[index3].second = additionModPrime[c[index3].second][xMinusR.second];
					}
				}
			}

			if (partyNum == PARTY_C)
			{
				for (int index2 = start; index2 < end; ++index2)
				{
					a = make_pair(0, 0);
					for (size_t k = 0; k < BIT_SIZE; ++k)
					{
						index3 = index2 * BIT_SIZE + k;
						// Complete Dot Product
						xMinusR.first = temp3[index3];
						xMinusR.second = recv[index3];

						// Resume rest of the loop
						c[index3] = a;
						tempM = share_m[index3];
						bit_r = (smallType)((r[index2] >> (BIT_SIZE - 1 - k)) & 1);

						tempN = XORPublicModPrime(tempM, bit_r);
						a = addModPrime(a, tempN);

						c[index3].first = additionModPrime[c[index3].first][xMinusR.first];
						c[index3].second = additionModPrime[c[index3].second][xMinusR.second];
						c[index3].second = additionModPrime[c[index3].second][1];
					}
				}
			}
		}

		// Private Compare functionality
		void funcPrivateCompare(const RSSVectorSmallType &share_m, const vector<myType> &r,
								const RSSVectorSmallType &beta, vector<smallType> &betaPrime,
								size_t size)
		{
			log_print("funcPrivateCompare");
			assert(share_m.size() == size * BIT_SIZE && "Input error share_m");
			assert(r.size() == size && "Input error r");
			assert(beta.size() == size && "Input error beta");

			size_t sizeLong = size * BIT_SIZE;
			size_t index3, index2;
			RSSVectorSmallType c(sizeLong), diff(sizeLong), twoBetaMinusOne(sizeLong), xMinusR(sizeLong);
			RSSSmallType a, tempM, tempN;
			smallType bit_r;

			// Computing x[i] - r[i]
			if (PARALLEL)
			{
				assert(NO_CORES > 2 && "Need at least 2 cores for threads variable abuse");
				vector<smallType> temp3(sizeLong, 0), recv(sizeLong, 0);

				// First part of parallel execution
				thread *threads = new thread[NO_CORES];
				int chunksize = size / NO_CORES;

				for (int i = 0; i < NO_CORES; i++)
				{
					int start = i * chunksize;
					int end = (i + 1) * chunksize;
					if (i == NO_CORES - 1)
						end = size;

					threads[i] = thread(parallelFirst, temp3.data(), beta.data(), r.data(),
										share_m.data(), start, end, i);
				}
				for (int i = 0; i < NO_CORES; i++)
					threads[i].join();

				//"Single" threaded execution
				threads[0] = thread(sendVector<smallType>, ref(temp3), prevParty(partyNum), size);
				threads[1] = thread(receiveVector<smallType>, ref(recv), nextParty(partyNum), size);

				for (int i = 0; i < 2; i++)
					threads[i].join();

				// Parallel execution resumes
				for (int i = 0; i < NO_CORES; i++)
				{
					int start = i * chunksize;
					int end = (i + 1) * chunksize;
					if (i == NO_CORES - 1)
						end = size;

					threads[i] = thread(parallelSecond, c.data(), temp3.data(), recv.data(),
										r.data(), share_m.data(), start, end, i);
				}

				for (int i = 0; i < NO_CORES; i++)
					threads[i].join();
				delete[] threads;
			}
			else
			{
				for (int index2 = 0; index2 < size; ++index2)
				{
					// Computing 2Beta-1
					twoBetaMinusOne[index2 * BIT_SIZE] = subConstModPrime(beta[index2], 1);
					twoBetaMinusOne[index2 * BIT_SIZE] = addModPrime(twoBetaMinusOne[index2 * BIT_SIZE], beta[index2]);

					for (size_t k = 0; k < BIT_SIZE; ++k)
					{
						index3 = index2 * BIT_SIZE + k;
						twoBetaMinusOne[index3] = twoBetaMinusOne[index2 * BIT_SIZE];

						bit_r = (smallType)((r[index2] >> (BIT_SIZE - 1 - k)) & 1);
						diff[index3] = share_m[index3];

						if (bit_r == 1)
							diff[index3] = subConstModPrime(diff[index3], 1);
					}
				}

				//(-1)^beta * x[i] - r[i]
				funcDotProduct(diff, twoBetaMinusOne, xMinusR, sizeLong);

				for (int index2 = 0; index2 < size; ++index2)
				{
					a = make_pair(0, 0);
					for (size_t k = 0; k < BIT_SIZE; ++k)
					{
						index3 = index2 * BIT_SIZE + k;
						c[index3] = a;
						tempM = share_m[index3];

						bit_r = (smallType)((r[index2] >> (BIT_SIZE - 1 - k)) & 1);

						tempN = XORPublicModPrime(tempM, bit_r);
						a = addModPrime(a, tempN);

						if (partyNum == PARTY_A)
						{
							c[index3].first = additionModPrime[c[index3].first][xMinusR[index3].first];
							c[index3].first = additionModPrime[c[index3].first][1];
							c[index3].second = additionModPrime[c[index3].second][xMinusR[index3].second];
						}
						else if (partyNum == PARTY_B)
						{
							c[index3].first = additionModPrime[c[index3].first][xMinusR[index3].first];
							c[index3].second = additionModPrime[c[index3].second][xMinusR[index3].second];
						}
						else if (partyNum == PARTY_C)
						{
							c[index3].first = additionModPrime[c[index3].first][xMinusR[index3].first];
							c[index3].second = additionModPrime[c[index3].second][xMinusR[index3].second];
							c[index3].second = additionModPrime[c[index3].second][1];
						}
					}
				}
			}

			RSSVectorSmallType temp_a(sizeLong / 2), temp_b(sizeLong / 2), temp_c(sizeLong / 2);
			vector<smallType> temp_d(sizeLong / 2);
			if (SECURITY_TYPE.compare("Malicious") == 0)
				funcCheckMaliciousDotProd(temp_a, temp_b, temp_c, temp_d, sizeLong / 2);

			// TODO 7 rounds of multiplication
			//  cout << "CM: \t\t" << funcTime(funcCrunchMultiply, c, betaPrime, size, dim) << endl;
			funcCrunchMultiply(c, betaPrime, size);
		}

		// Wrap functionality.
		void funcWrap(const RSSVectorMyType &a, RSSVectorSmallType &theta, size_t size)
		{
			log_print("funcWrap");

			size_t sizeLong = size * BIT_SIZE;
			RSSVectorMyType x(size), r(size);
			RSSVectorSmallType shares_r(sizeLong), alpha(size), beta(size), eta(size);
			vector<smallType> delta(size), etaPrime(size);
			vector<myType> reconst_x(size);

			PrecomputeObject.getShareConvertObjects(r, shares_r, alpha, size);
			addVectors<RSSMyType>(a, r, x, size);
			for (int i = 0; i < size; ++i)
			{
				beta[i].first = wrapAround(a[i].first, r[i].first);
				x[i].first = a[i].first + r[i].first;
				beta[i].second = wrapAround(a[i].second, r[i].second);
				x[i].second = a[i].second + r[i].second;
			}

			vector<myType> x_next(size), x_prev(size);
			for (int i = 0; i < size; ++i)
			{
				x_prev[i] = 0;
				x_next[i] = x[i].first;
				reconst_x[i] = x[i].first;
				reconst_x[i] = reconst_x[i] + x[i].second;
			}

			thread *threads = new thread[2];
			threads[0] = thread(sendVector<myType>, ref(x_next), nextParty(partyNum), size);
			threads[1] = thread(receiveVector<myType>, ref(x_prev), prevParty(partyNum), size);
			for (int i = 0; i < 2; i++)
				threads[i].join();
			delete[] threads;

			for (int i = 0; i < size; ++i)
				reconst_x[i] = reconst_x[i] + x_prev[i];

			wrap3(x, x_prev, delta, size); // All parties have delta
			PrecomputeObject.getRandomBitShares(eta, size);

			// cout << "PC: \t\t" << funcTime(funcPrivateCompare, shares_r, reconst_x, eta, etaPrime, size, BIT_SIZE) << endl;
			funcPrivateCompare(shares_r, reconst_x, eta, etaPrime, size);

			if (partyNum == PARTY_A)
			{
				for (int i = 0; i < size; ++i)
				{
					theta[i].first = beta[i].first ^ delta[i] ^ alpha[i].first ^ eta[i].first ^ etaPrime[i];
					theta[i].second = beta[i].second ^ alpha[i].second ^ eta[i].second;
				}
			}
			else if (partyNum == PARTY_B)
			{
				for (int i = 0; i < size; ++i)
				{
					theta[i].first = beta[i].first ^ delta[i] ^ alpha[i].first ^ eta[i].first;
					theta[i].second = beta[i].second ^ alpha[i].second ^ eta[i].second;
				}
			}
			else if (partyNum == PARTY_C)
			{
				for (int i = 0; i < size; ++i)
				{
					theta[i].first = beta[i].first ^ alpha[i].first ^ eta[i].first;
					theta[i].second = beta[i].second ^ delta[i] ^ alpha[i].second ^ eta[i].second ^ etaPrime[i];
				}
			}
		}

		// Set c[i] = a[i] if b[i] = 0
		// Set c[i] = 0    if b[i] = 1
		void funcSelectShares(const RSSVectorMyType &a, const RSSVectorSmallType &b,
							  RSSVectorMyType &selected, size_t size)
		{
			log_print("funcSelectShares");

			RSSVectorSmallType c(size), bXORc(size);
			RSSVectorMyType m_c(size);
			vector<smallType> reconst_b(size);
			PrecomputeObject.getSelectorBitShares(c, m_c, size);

			for (int i = 0; i < size; ++i)
			{
				bXORc[i].first = c[i].first ^ b[i].first;
				bXORc[i].second = c[i].second ^ b[i].second;
			}

			funcReconstructBit(bXORc, reconst_b, size, "bXORc", false);

			if (partyNum == PARTY_A)
				for (int i = 0; i < size; ++i)
					if (reconst_b[i] == 0)
					{
						m_c[i].first = (myType)1 - m_c[i].first;
						m_c[i].second = -m_c[i].second;
					}

			if (partyNum == PARTY_B)
				for (int i = 0; i < size; ++i)
					if (reconst_b[i] == 0)
					{
						m_c[i].first = -m_c[i].first;
						m_c[i].second = -m_c[i].second;
					}

			if (partyNum == PARTY_C)
				for (int i = 0; i < size; ++i)
					if (reconst_b[i] == 0)
					{
						m_c[i].first = -m_c[i].first;
						m_c[i].second = (myType)1 - m_c[i].second;
					}

			funcDotProduct(a, m_c, selected, size, false, 0);
		}

		// Within each group of columns, select a0 or a1 depending on value of bit b into answer.
		// loopCounter is used to offset a1 by loopCounter*rows*columns
		// answer = ((a0 \oplus a1) b ) \oplus a0
		void funcSelectBitShares(const RSSVectorSmallType &a0, const RSSVectorSmallType &a1,
								 const RSSVectorSmallType &b, RSSVectorSmallType &answer,
								 size_t rows, size_t columns, size_t loopCounter)
		{
			log_print("funcSelectBitShares");
			size_t size = rows * columns;
			assert(a0.size() == rows * columns && "a0 size incorrect");
			assert(a1.size() == (columns)*rows * columns && "a1 size incorrect");
			assert(b.size() == rows && "b size incorrect");
			assert(answer.size() == rows * columns && "answers size incorrect");

			RSSVectorSmallType bRepeated(size), tempXOR(size);
			for (int i = 0; i < rows; ++i)
				for (size_t j = 0; j < columns; ++j)
					bRepeated[i * columns + j] = b[i];

			for (size_t i = 0; i < rows; ++i)
				for (size_t j = 0; j < columns; ++j)
					tempXOR[i * columns + j] = a0[i * columns + j] ^
											   a1[loopCounter * rows * columns + i * columns + j];

			funcDotProductBits(tempXOR, bRepeated, answer, size);

			for (int i = 0; i < size; ++i)
				answer[i] = answer[i] ^ a0[i];
		}

		// b holds bits of ReLU' of a
		void funcRELUPrime(const RSSVectorMyType &a, RSSVectorSmallType &b, size_t size)
		{
			log_print("funcRELUPrime");

			RSSVectorMyType twoA(size);
			RSSVectorSmallType theta(size);
			for (int i = 0; i < size; ++i)
				twoA[i] = a[i] << 1;

			// cout << "Wrap: \t\t" << funcTime(funcWrap, twoA, theta, size) << endl;
			funcWrap(twoA, theta, size);

			for (int i = 0; i < size; ++i)
			{
				b[i].first = theta[i].first ^ (getMSB(a[i].first));
				b[i].second = theta[i].second ^ (getMSB(a[i].second));
			}
		}

		// Input is a, outputs are temp = ReLU'(a) and b = RELU(a).
		void funcRELU(const RSSVectorMyType &a, RSSVectorSmallType &temp, RSSVectorMyType &b, size_t size)
		{
			log_print("funcRELU");

			RSSVectorSmallType c(size), bXORc(size);
			RSSVectorMyType m_c(size);
			vector<smallType> reconst_b(size);

			// cout << "ReLU': \t\t" << funcTime(funcRELUPrime, a, temp, size) << endl;
			funcRELUPrime(a, temp, size);
			PrecomputeObject.getSelectorBitShares(c, m_c, size);

			for (int i = 0; i < size; ++i)
			{
				bXORc[i].first = c[i].first ^ temp[i].first;
				bXORc[i].second = c[i].second ^ temp[i].second;
			}

			funcReconstructBit(bXORc, reconst_b, size, "bXORc", false);
			if (partyNum == PARTY_A)
				for (int i = 0; i < size; ++i)
					if (reconst_b[i] == 0)
					{
						m_c[i].first = (myType)1 - m_c[i].first;
						m_c[i].second = -m_c[i].second;
					}

			if (partyNum == PARTY_B)
				for (int i = 0; i < size; ++i)
					if (reconst_b[i] == 0)
					{
						m_c[i].first = -m_c[i].first;
						m_c[i].second = -m_c[i].second;
					}

			if (partyNum == PARTY_C)
				for (int i = 0; i < size; ++i)
					if (reconst_b[i] == 0)
					{
						m_c[i].first = -m_c[i].first;
						m_c[i].second = (myType)1 - m_c[i].second;
					}

			// vector<myType> reconst_m_c(size);
			// funcReconstruct(m_c, reconst_m_c, size, "m_c", true);
			funcDotProduct(a, m_c, b, size, false, 0);
		}

		void funcPow(const RSSVectorMyType &b, vector<smallType> &alpha, size_t size)
		{
			size_t ell = 5;
			if (BIT_SIZE == 64)
				ell = 6;

			RSSVectorMyType x(size), d(size), temp(size);
			copyVectors<RSSMyType>(b, x, size);

			RSSVectorSmallType c(size);
			for (int i = 0; i < size; ++i)
				alpha[i] = 0;

			vector<smallType> r_c(size);
			// TODO vecrorize this, right now only accepts the first argument
			for (int j = ell - 1; j > -1; --j)
			{
				vector<myType> temp_1(size, (1 << ((1 << j) + (int)alpha[0])));
				funcGetShares(temp, temp_1);
				subtractVectors<RSSMyType>(x, temp, d, size);
				funcRELUPrime(d, c, size);
				funcReconstructBit(c, r_c, size, "null", false);
				if (r_c[0] == 0)
				{
					for (int i = 0; i < size; ++i)
						alpha[i] += (1 << j);
				}
			}
		}

		// All parties start with shares of a number in a and b and the quotient is in quotient.
		// alpha is the order of divisiors, 2^alpha =< b < 2^{alpha+1}.
		void funcDivision(const RSSVectorMyType &a, const RSSVectorMyType &b, RSSVectorMyType &quotient,
						  size_t size)
		{
			log_print("funcDivision");

			// TODO incorporate funcPow
			// TODO Scale up and complete this computation with fixed-point precision
			vector<smallType> alpha_temp(size);
			funcPow(b, alpha_temp, size);

			size_t alpha = alpha_temp[0];
			size_t precision = alpha + 1;
			const myType constTwoPointNine = ((myType)(2.9142 * (1 << precision)));
			const myType constOne = ((myType)(1 * (1 << precision)));

			vector<myType> data_twoPointNine(size, constTwoPointNine), data_one(size, constOne), reconst(size);
			RSSVectorMyType ones(size), twoPointNine(size), twoX(size), w0(size), xw0(size),
				epsilon0(size), epsilon1(size), termOne(size), termTwo(size), answer(size);
			funcGetShares(twoPointNine, data_twoPointNine);
			funcGetShares(ones, data_one);

			multiplyByScalar(b, 2, twoX);
			subtractVectors<RSSMyType>(twoPointNine, twoX, w0, size);
			funcDotProduct(b, w0, xw0, size, true, precision);
			subtractVectors<RSSMyType>(ones, xw0, epsilon0, size);
			if (PRECISE_DIVISION)
				funcDotProduct(epsilon0, epsilon0, epsilon1, size, true, precision);
			addVectors(ones, epsilon0, termOne, size);
			if (PRECISE_DIVISION)
				addVectors(ones, epsilon1, termTwo, size);
			funcDotProduct(w0, termOne, answer, size, true, precision);
			if (PRECISE_DIVISION)
				funcDotProduct(answer, termTwo, answer, size, true, precision);

			// RSSVectorMyType scaledA(size);
			// multiplyByScalar(a, (1 << (alpha + 1)), scaledA);
			funcDotProduct(answer, a, quotient, size, true, ((2 * precision - FLOAT_PRECISION)));
		}

		// a is of size batchSize*B, b is of size B and quotient is a/b (b from each group).
		void funcBatchNorm(const RSSVectorMyType &a, const RSSVectorMyType &b, RSSVectorMyType &quotient,
						   size_t batchSize, size_t B)
		{
			log_print("funcBatchNorm");
			// TODO Scale up and complete this computation with higher fixed-point precision

			assert(a.size() == batchSize * B && "funcBatchNorm a size incorrect");
			assert(b.size() == B && "funcBatchNorm b size incorrect");
			assert(quotient.size() == batchSize * B && "funcBatchNorm quotient size incorrect");

			vector<smallType> alpha_temp(B);
			funcPow(b, alpha_temp, B);

			// TODO Get alpha from previous computation
			size_t alpha = alpha_temp[0];
			size_t precision = alpha + 1;
			const myType constTwoPointNine = ((myType)(2.9142 * (1 << precision)));
			const myType constOne = ((myType)(1 * (1 << precision)));

			vector<myType> data_twoPointNine(B, constTwoPointNine), data_one(B, constOne), reconst(B);
			RSSVectorMyType ones(B), twoPointNine(B), twoX(B), w0(B), xw0(B),
				epsilon0(B), epsilon1(B), termOne(B), termTwo(B), answer(B);
			funcGetShares(twoPointNine, data_twoPointNine);
			funcGetShares(ones, data_one);

			multiplyByScalar(b, 2, twoX);
			subtractVectors<RSSMyType>(twoPointNine, twoX, w0, B);
			funcDotProduct(b, w0, xw0, B, true, precision);
			subtractVectors<RSSMyType>(ones, xw0, epsilon0, B);
			if (PRECISE_DIVISION)
				funcDotProduct(epsilon0, epsilon0, epsilon1, B, true, precision);
			addVectors(ones, epsilon0, termOne, B);
			if (PRECISE_DIVISION)
				addVectors(ones, epsilon1, termTwo, B);
			funcDotProduct(w0, termOne, answer, B, true, precision);
			if (PRECISE_DIVISION)
				funcDotProduct(answer, termTwo, answer, B, true, precision);

			RSSVectorMyType scaledA(batchSize * B), b_repeat(batchSize * B);
			// multiplyByScalar(a, 2, scaledA); //So that a is of precision precision
			for (int i = 0; i < B; ++i)
				for (int j = 0; j < batchSize; ++j)
					b_repeat[i * batchSize + j] = answer[i];
			funcDotProduct(b_repeat, a, quotient, batchSize * B, true, (2 * precision - FLOAT_PRECISION)); // Convert to fixed precision
		}

		// Chunk wise maximum of a vector of size rows*columns and maximum is caclulated of every
		// column number of elements. max is a vector of size rows, maxPrime, of rows*columns*columns;
		void funcMaxpool(RSSVectorMyType &a, RSSVectorMyType &max, RSSVectorSmallType &maxPrime,
						 size_t rows, size_t columns)
		{
			log_print("funcMaxpool");
			assert(columns < 256 && "Pooling size has to be smaller than 8-bits");

			size_t size = rows * columns;
			RSSVectorMyType diff(rows);
			RSSVectorSmallType rp(rows), dmpIndexShares(columns * size), temp(size);
			vector<smallType> dmpTemp(columns * size, 0);

			for (int loopCounter = 0; loopCounter < columns; ++loopCounter)
				for (size_t i = 0; i < rows; ++i)
					dmpTemp[loopCounter * rows * columns + i * columns + loopCounter] = 1;
			funcGetShares(dmpIndexShares, dmpTemp);

			for (size_t i = 0; i < size; ++i)
				maxPrime[i] = dmpIndexShares[i];

			for (size_t i = 0; i < rows; ++i)
				max[i] = a[i * columns];

			for (size_t i = 1; i < columns; ++i)
			{
				for (size_t j = 0; j < rows; ++j)
					diff[j] = max[j] - a[j * columns + i];

				funcRELU(diff, rp, max, rows);
				funcSelectBitShares(maxPrime, dmpIndexShares, rp, temp, rows, columns, i);

				for (size_t i = 0; i < size; ++i)
					maxPrime[i] = temp[i];

				for (size_t j = 0; j < rows; ++j)
					max[j] = max[j] + a[j * columns + i];
			}
		}

		/****************************************************************/
		/* 							DEBUG 								*/
		/****************************************************************/

		/* This is quite interesting, negatives move more towards the exact
		computation. So it might be the case that the approximations
		introduced by negative and positive numbers in effect cancel out to
		preserve overall NN accuracy. */
		void debugMatMul()
		{
			// size_t rows = 1000;
			// size_t common_dim = 1000;
			// size_t columns = 1000;
			// size_t transpose_a = 0, transpose_b = 0;

			// RSSVectorMyType a(rows*common_dim, make_pair(1,1)),
			// 				b(common_dim*columns, make_pair(1,1)), c(rows*columns);

			// funcMatMul(a, b, c, rows, common_dim, columns, transpose_a, transpose_b, FLOAT_PRECISION);

			/******************************** Attempt 1 ****************************************/
			// 	size_t rows = 3;
			// 	size_t common_dim = 2;
			// 	size_t columns = 3;
			// 	size_t transpose_a = 0, transpose_b = 0;

			// 	RSSVectorMyType a(rows*common_dim), b(common_dim*columns), c(rows*columns);
			// 	vector<myType> a_reconst(rows*columns), b_reconst(common_dim*columns), c_reconst(rows*columns);

			// 	vector<myType> data_a = {floatToMyType(3),floatToMyType(4),
			// 							 floatToMyType(5),floatToMyType(6),
			// 							 floatToMyType(7),floatToMyType(8)};
			// 	vector<myType> data_b = {floatToMyType(4),floatToMyType(5),floatToMyType(6),
			// 							 floatToMyType(7),floatToMyType(8),floatToMyType(9)};
			// 	funcGetShares(a, data_a);
			// 	funcGetShares(b, data_b);

			// 	funcMatMul(a, b, c, rows, common_dim, columns, transpose_a, transpose_b, FLOAT_PRECISION);

			// #if (!LOG_DEBUG)
			// 	funcReconstruct(a, a_reconst, rows*common_dim, "a", true);
			// 	funcReconstruct(b, b_reconst, common_dim*columns, "b", true);
			// 	funcReconstruct(c, c_reconst, rows*columns, "c", true);
			// #endif

			/******************************** Attemp2 ****************************************/
			size_t rows = 2;
			size_t common_dim = 2;
			size_t columns = 2;
			size_t transpose_a = 0, transpose_b = 0;

			RSSVectorMyType a(rows * common_dim), b(common_dim * columns), c(rows * columns);
			// vector<myType> a_reconst(rows*columns), b_reconst(common_dim*columns), c_reconst(rows*columns);

			// A, B 		: 0001011001110, 1000011000101, 0110001100111, 0110101000001
			// 				: 718, 4293, 3175, 3393
			// C(exact)  	: 14145799/67108864, 17648523/67108864, 13052425/67108864, 6285681/16777216
			// C(MPC)		: 1726, 2154, 1593, 3069
			// Float(exact)	: [0.21069336, 0.26293945, 0.19445801, 0.37463379]
			// Float(MPC)	: [0.21078883, 0.26298349, 0.19449629, 0.37465578]
			// Rel error (%)	: [0.04531312, 0.01674641, 0.01968604, 0.00587083] --> 0.022% error
			vector<myType> data_a = {static_cast<myType>(-718),
									 static_cast<myType>(-4293),
									 static_cast<myType>(-3175),
									 static_cast<myType>(-3393)};

			vector<myType> data_b = {static_cast<myType>(718),
									 static_cast<myType>(4293),
									 static_cast<myType>(3175),
									 static_cast<myType>(3393)};
			funcGetShares(a, data_a);
			funcGetShares(b, data_b);

			funcMatMul(a, b, c, rows, common_dim, columns, transpose_a, transpose_b, FLOAT_PRECISION);

			print_vector(a, "FLOAT", "a", a.size());
			print_vector(b, "FLOAT", "b", b.size());
			print_vector(c, "FLOAT", "c", c.size());
			// #if (!LOG_DEBUG)
			// 	funcReconstruct(a, a_reconst, rows*common_dim, "a", true);
			// 	funcReconstruct(b, b_reconst, common_dim*columns, "b", true);
			// 	funcReconstruct(c, c_reconst, rows*columns, "c", true);
			// #endif

			/******************************** Attemp3 ****************************************/
			// 	size_t rows = 1;
			// 	size_t common_dim = 1;
			// 	size_t columns = 1;
			// 	size_t transpose_a = 0, transpose_b = 0;

			// 	RSSVectorMyType a(rows*common_dim), b(common_dim*columns), c(rows*columns);

			// // A, B 		: 0001011001110, 1000011000101, 0110001100111, 0110101000001
			// // 				: 718, 4293, 3175, 3393
			// // C(exact)  	: 14145799/67108864, 17648523/67108864, 13052425/67108864, 6285681/16777216
			// // C(MPC)		: 1726, 2154, 1593, 3069
			// //Float(exact)	: [0.21069336, 0.26293945, 0.19445801, 0.37463379]
			// //Float(MPC)	: [0.21078883, 0.26298349, 0.19449629, 0.37465578]
			// //Rel error (%)	: [0.04531312, 0.01674641, 0.01968604, 0.00587083] --> 0.022% error
			// 	vector<myType> data_a = {-718};
			// 	vector<myType> data_b = {718};
			// 	funcGetShares(a, data_a);
			// 	funcGetShares(b, data_b);

			// 	funcMatMul(a, b, c, rows, common_dim, columns, transpose_a, transpose_b, FLOAT_PRECISION);

			// 	print_vector(a, "FLOAT", "a", a.size());
			// 	print_vector(b, "FLOAT", "b", b.size());
			// 	print_vector(c, "FLOAT", "c", c.size());

			/******************************** Attemp4 ****************************************/
			// size_t rows = 1;
			// size_t common_dim = 1;
			// size_t columns = 1;
			// size_t transpose_a = 0, transpose_b = 0;

			// RSSVectorMyType a(rows*common_dim), b(common_dim*columns), c(rows*columns);

			// vector<myType> data_a = {-718};
			// vector<myType> data_b = {718};
			// funcGetShares(a, data_a);
			// funcGetShares(b, data_b);

			// myType result = a[0].first * b[0].first +
			//                    a[0].first * b[0].second +
			//                    a[0].second * b[0].first;

			// // print_linear(data_a[0], "SIGNED");
			// // print_linear(result, "SIGNED");
			// print_linear(dividePlain(result, (1 << FLOAT_PRECISION)), "SIGNED");
		}

		void debugDotProd()
		{
			/****************************** myType ***************************/
			// size_t rows = 3;
			// size_t columns = 3;

			// RSSVectorMyType a(rows*columns, make_pair(0,0)),
			// 				b(rows*columns, make_pair(0,0)),
			// 				c(rows*columns);
			// vector<myType> a_reconst(rows*columns), b_reconst(rows*columns), c_reconst(rows*columns);

			// vector<myType> data = {floatToMyType(3),floatToMyType(4),floatToMyType(5),
			// 						 floatToMyType(6),floatToMyType(7),floatToMyType(8),
			// 						 floatToMyType(7),floatToMyType(8),floatToMyType(9)};
			// funcAddConstant(a, data);
			// funcAddConstant(b, data);

			// funcReconstruct(a, a_reconst, rows*columns, "a", true);
			// funcReconstruct(b, b_reconst, rows*columns, "b", true);
			// funcDotProduct(a, b, c, rows*columns, true, FLOAT_PRECISION);
			// funcReconstruct(c, c_reconst, rows*columns, "c", true);

			/****************************** smallType ***************************/
			size_t size = 9;

			RSSVectorSmallType a(size, make_pair(1, 1)),
				b(size, make_pair(1, 1)),
				c(size);

			funcDotProduct(a, b, c, size);
		}

		void debugPC()
		{
			vector<myType> plain_m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
			vector<myType> plain_r{10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
			vector<smallType> plain_beta{1, 0, 1, 0, 0, 0, 1, 1, 0, 1};
			size_t size = plain_m.size();
			size_t sizeLong = size * BIT_SIZE;
			assert(plain_r.size() == plain_m.size() && "Error in debugPC");

			RSSVectorSmallType beta(size), shares_m(sizeLong);
			vector<smallType> reconst_betaP(size), betaPrime(size);
			funcGetShares(beta, plain_beta);

			vector<smallType> bits_of_m(sizeLong);
			for (int i = 0; i < size; ++i)
				for (int j = 0; j < BIT_SIZE; ++j)
					bits_of_m[i * BIT_SIZE + j] = (smallType)((plain_m[i] >> (BIT_SIZE - 1 - j)) & 1);

			funcGetShares(shares_m, bits_of_m);
			funcPrivateCompare(shares_m, plain_r, beta, betaPrime, size);

#if (!LOG_DEBUG)
			cout << "BetaPrime: \t ";
			for (int i = 0; i < size; ++i)
				cout << (int)betaPrime[i] << " ";
			cout << endl;
			cout << "Beta: \t\t";
			for (int i = 0; i < size; ++i)
				cout << (int)plain_beta[i] << " ";
			cout << endl;
			cout << "m: \t\t";
			for (int i = 0; i < size; ++i)
				cout << (int)plain_m[i] << " ";
			cout << endl;
			cout << "r: \t\t";
			for (int i = 0; i < size; ++i)
				cout << (int)plain_r[i] << " ";
			cout << endl;
			cout << "m-r: \t\t";
			for (int i = 0; i < size; ++i)
				cout << (int)plain_m[i] - (int)plain_r[i] << " ";
			cout << endl;
#endif
		}

		void debugWrap()
		{
			size_t size = 5;
			RSSVectorMyType a(size);
			RSSVectorSmallType theta(size);
			vector<smallType> reconst(size);

			myType interesting = MINUS_ONE / 3;
			a[0] = make_pair(0, 0);
			a[1] = make_pair(interesting, interesting);
			interesting += 1;
			a[2] = make_pair(interesting, interesting);
			interesting = ((MINUS_ONE / 3) << 1);
			a[3] = make_pair(interesting, interesting);
			interesting += 1;
			a[4] = make_pair(interesting, interesting);

			funcWrap(a, theta, size);

#if (!LOG_DEBUG)
			cout << "a: \t\t ";
			for (int i = 0; i < size; ++i)
				cout << (int)3 * a[i].first << "(" << (int)a[i].first << ", " << (int)a[i].second << ") ";
			cout << endl;
			funcReconstruct(theta, reconst, size, "Theta", true);
#endif
		}

		void debugReLUPrime()
		{
			vector<myType> data_a = {static_cast<myType>(1),
									 static_cast<myType>(2),
									 static_cast<myType>(-1),
									 static_cast<myType>(-2),
									 static_cast<myType>(3)};
			size_t size = data_a.size();
			RSSVectorMyType a(size);
			RSSVectorSmallType b(size);
			vector<myType> reconst_a(size);
			vector<smallType> reconst_b(size);

			funcGetShares(a, data_a);
			funcRELUPrime(a, b, size);

#if (!LOG_DEBUG)
			funcReconstruct(a, reconst_a, size, "a", true);
			funcReconstructBit(b, reconst_b, size, "b", true);
#endif
		}

		void debugReLU()
		{
			vector<myType> data_a{0, 1, 2, 3, 4, 5, 6, 7};
			size_t size = data_a.size();
			RSSVectorMyType a(size), b(size);
			RSSVectorSmallType aPrime(size);
			vector<myType> reconst_b(size);

			funcGetShares(a, data_a);
			for (int i = size / 2; i < size; ++i)
			{
				a[i].first = a[i].first << BIT_SIZE - 3;
				a[i].second = a[i].second << BIT_SIZE - 3;
			}

			funcRELU(a, aPrime, b, size);

#if (!LOG_DEBUG)
			funcReconstruct(a, data_a, size, "a", true);
			funcReconstruct(b, reconst_b, size, "ReLU", true);
#endif
		}

		void debugDivision()
		{
			vector<myType> data_a = {floatToMyType(3.2)}, data_b = {floatToMyType(9.7)};
			size_t size = data_a.size();
			RSSVectorMyType a(size), b(size), quotient(size);
			vector<myType> reconst(size);

			funcGetShares(a, data_a);
			funcGetShares(b, data_b);
			funcDivision(a, b, quotient, size);

#if (!LOG_DEBUG)
			funcReconstruct(a, reconst, size, "a", true);
			funcReconstruct(b, reconst, size, "b", true);
			funcReconstruct(quotient, reconst, size, "Quot", true);
			print_myType(reconst[0], "Quotient[0]", "FLOAT");
#endif
		}

		// TODO This needs thorough debugging. Many issues with this function.
		void debugBN()
		{
			// vector<myType> data_a = {1<<13}, data_b = {(1<<14) + (1<<13)};
			// Values over and above 10, use lower fixed-point precision. 13 bits + another 2-3 powers of 2 is too much.
			vector<myType> data_a = {floatToMyType(3.2)}, data_b = {floatToMyType(9.8)};
			size_t size = data_a.size();
			RSSVectorMyType a(size), b(size), quotient(size);
			vector<myType> reconst(size);

			funcGetShares(a, data_a);
			funcGetShares(b, data_b);
			funcBatchNorm(a, b, quotient, 1, 1);

#if (!LOG_DEBUG)
			funcReconstruct(a, reconst, size, "a", true);
			funcReconstruct(b, reconst, size, "b", true);
			funcReconstruct(quotient, reconst, size, "Quot", true);
			print_myType(reconst[0], "Quotient[0]", "FLOAT");
#endif
		}

		void debugSSBits()
		{
			size_t rows = 4;
			size_t columns = 3;
			vector<smallType> a0 = {1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0};
			vector<smallType> a1 = {0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,
									1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1,
									1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1};
			vector<smallType> rp = {0, 0, 1, 1};
			RSSVectorSmallType x(rows * columns), y(rows * columns * columns), z(rows), answer(rows * columns);
			funcGetShares(x, a0);
			funcGetShares(y, a1);
			funcGetShares(z, rp);

#if (!LOG_DEBUG)
			funcReconstructBit(x, a1, x.size(), "x", true);
			funcReconstructBit(y, a1, y.size(), "y", true);
			funcReconstructBit(z, a1, z.size(), "z", true);
#endif

			for (int i = 0; i < 3; ++i)
			{
				funcSelectBitShares(x, y, z, answer, rows, columns, i);
#if (!LOG_DEBUG)
				funcReconstructBit(answer, a1, answer.size(), "a", true);
#endif
			}
		}

		void debugSS()
		{
			// vector<smallType> bits = {1,0,0,1,1,1,0,1,1,0};
			// Make clang happy.
			vector<smallType> bits;
			bits.push_back(1);
			bits.push_back(0);
			bits.push_back(0);
			bits.push_back(1);
			bits.push_back(1);
			bits.push_back(1);
			bits.push_back(0);
			bits.push_back(1);
			bits.push_back(1);
			bits.push_back(0);

			size_t size = bits.size();
			// vector<myType> data = {1,29,10,2938,27,-1,-23,12,2,571}, reconst(size);
			vector<myType> data, reconst(size);
			// Make clang happy.
			data.push_back(1);
			data.push_back(29);
			data.push_back(10);
			data.push_back(2938);
			data.push_back(27);
			data.push_back(-1);
			data.push_back(-23);
			data.push_back(12);
			data.push_back(2);
			data.push_back(571);

			assert(size == data.size() && "Size mismatch");
			RSSVectorMyType a(size), selection(size);
			RSSVectorSmallType b(size);

			funcGetShares(a, data);
			funcGetShares(b, bits);

			funcSelectShares(a, b, selection, size);

#if (!LOG_DEBUG)
			funcReconstruct(a, reconst, size, "a", true);
			funcReconstructBit(b, bits, size, "b", true);
			funcReconstruct(selection, reconst, size, "Sel'd", true);
#endif
		}

		void debugMaxpool()
		{
			size_t rows = 5;
			size_t columns = 3;
			size_t size = rows * columns;
			// vector<myType> data = {1,2,3,
			// 					   3,1,2,
			// 					   1,5,3,
			// 					   5,1,6,
			// 					   6,3,9}, reconst(size);
			// Make clang happy.
			vector<myType> data, reconst(size);
			data.push_back(1);
			data.push_back(2);
			data.push_back(3);
			data.push_back(3);
			data.push_back(1);
			data.push_back(2);
			data.push_back(1);
			data.push_back(5);
			data.push_back(3);
			data.push_back(5);
			data.push_back(1);
			data.push_back(6);
			data.push_back(6);
			data.push_back(3);
			data.push_back(9);

			RSSVectorMyType a(size), max(rows);
			RSSVectorSmallType maxPrime(rows * columns);
			vector<smallType> reconst_maxPrime(maxPrime.size());
			funcGetShares(a, data);
			funcMaxpool(a, max, maxPrime, rows, columns);

#if (!LOG_DEBUG)
			funcReconstruct(a, reconst, size, "a", true);
			funcReconstruct(max, reconst, rows, "val", true);
			funcReconstructBit(maxPrime, reconst_maxPrime, rows * columns, "maxP", true);
#endif
		}

		/******************************** Test ********************************/

		void testMatMul(size_t rows, size_t common_dim, size_t columns, size_t iter)
		{

			/******************************** TODO ****************************************/
			RSSVectorMyType a(rows * common_dim, make_pair(0, 0));
			RSSVectorMyType b(common_dim * columns, make_pair(0, 0));
			RSSVectorMyType c(rows * columns);

			for (int runs = 0; runs < iter; ++runs)
				funcMatMul(a, b, c, rows, common_dim, columns, 0, 0, FLOAT_PRECISION);
		}

		void testConvolution(size_t iw, size_t ih, size_t Din, size_t Dout,
							 size_t f, size_t S, size_t P, size_t B, size_t iter)
		{
			// size_t ow 		= (((iw-f+2*P)/S)+1);
			// size_t oh		= (((ih-f+2*P)/S)+1);
			// size_t tempSize = ow*oh;

			// RSSVectorMyType a(iw*ih*Din*B, make_pair(0,0));
			// RSSVectorMyType b(f*f*Din*Dout, make_pair(0,0));
			// RSSVectorMyType ans(ow*oh*Dout*B, make_pair(0,0));
			// RSSVectorMyType c(Dout, make_pair(0,0));

			// for (int runs = 0; runs < iter; ++runs)
			// {
			// 	//Reshape activations
			// 	RSSVectorMyType temp1((iw+2*P)*(ih+2*P)*Din*B, make_pair(0,0));
			// 	zeroPad(a, temp1, iw, ih, P, Din, B);

			// 	//Reshape for convolution
			// 	RSSVectorMyType temp2((f*f*Din) * (ow * oh * B));
			// 	convToMult(temp1, temp2, (iw+2*P), (ih+2*P), f, Din, S, B);

			// 	//Perform the multiplication, transpose the actications.
			// 	RSSVectorMyType temp3(Dout * (ow*oh*B));
			// 	funcMatMul(b, temp2, temp3, Dout, (f*f*Din), (ow*oh*B), 0, 1, FLOAT_PRECISION);

			// 	//Add biases and meta-transpose
			// 	for (size_t i = 0; i < B; ++i)
			// 		for (size_t j = 0; j < Dout; ++j)
			// 			for (size_t k = 0; k < tempSize; ++k)
			// 				ans[i*Dout*tempSize + j*tempSize + k]
			// 					= temp3[j*B*tempSize + i*tempSize + k] + c[j];
			// }
		}

		void testRelu(size_t r, size_t c, size_t iter)
		{
			RSSVectorMyType a(r * c, make_pair(0, 0));
			RSSVectorSmallType reluPrime(r * c);
			RSSVectorMyType b(r * c);

			for (int runs = 0; runs < iter; ++runs)
				funcRELU(a, reluPrime, b, r * c);
		}

		void testReluPrime(size_t r, size_t c, size_t iter)
		{
			RSSVectorMyType a(r * c, make_pair(0, 0));
			RSSVectorSmallType reluPrime(r * c);

			for (int runs = 0; runs < iter; ++runs)
				funcRELUPrime(a, reluPrime, r * c);
		}

		void testMaxpool(size_t ih, size_t iw, size_t Din, size_t f, size_t S, size_t B, size_t iter)
		{
			size_t ow = (((iw - f) / S) + 1);
			size_t oh = (((ih - f) / S) + 1);

			RSSVectorMyType a(iw * ih * Din * B);
			RSSVectorMyType b(ow * oh * Din * B);
			RSSVectorSmallType c(iw * ih * Din * B);
			RSSVectorMyType temp1(ow * oh * Din * B * f * f);
			size_t sizeBeta = iw;
			size_t sizeD = sizeBeta * ih;
			size_t sizeB = sizeD * Din;
			size_t counter = 0;

			for (int runs = 0; runs < iter; ++runs)
			{
				counter = 0;
				for (int b = 0; b < B; ++b)
					for (size_t r = 0; r < Din; ++r)
						for (size_t beta = 0; beta < ih - f + 1; beta += S)
							for (size_t alpha = 0; alpha < iw - f + 1; alpha += S)
								for (int q = 0; q < f; ++q)
									for (int p = 0; p < f; ++p)
									{
										temp1[counter++] =
											a[b * sizeB + r * sizeD + (beta + q) * sizeBeta + (alpha + p)];
									}
				// Pooling operation
				funcMaxpool(temp1, b, c, ow * oh * Din * B, f * f);
			}
		}
	} // namespace primihub{
}