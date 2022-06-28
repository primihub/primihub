
#include "tools.h"
#include <stdint.h>
#include <mutex>
#include <bitset>
#include "EigenMatMul.h"

using namespace std;
namespace primihub
{
	namespace falcon
	{
		using namespace Eigen;
		smallType additionModPrime[PRIME_NUMBER][PRIME_NUMBER];
		smallType subtractModPrime[PRIME_NUMBER][PRIME_NUMBER];
		smallType multiplicationModPrime[PRIME_NUMBER][PRIME_NUMBER];

		/* Here are the global variables, precomputed once in order to save time*/
		// powers[x*deg+(i-1)]=x^i, i.e., POW(SETX(X),i). Notice the (i-1), due to POW(x,0) not saved (always 1...)
		__m128i *powers;
		// the coefficients used in the reconstruction
		__m128i *baseReduc;
		__m128i *baseRecon;
		// allocation of memory for polynomial coefficients
		__m128i *polyCoef;
		// one in the field
		const __m128i ONE = SET_ONE;
		// zero in the field
		const __m128i ZERO = SET_ZERO;
		// the constant used for BGW step by this party
		__m128i BGWconst;

		__m128i *sharesTest;

#ifdef ENABLE_SSE
		void gfmul(__m128i a, __m128i b, __m128i *res)
		{
			__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6,
				tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
			__m128i XMMMASK = _mm_setr_epi32(0xffffffff, 0x0, 0x0, 0x0);
			tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
			tmp6 = _mm_clmulepi64_si128(a, b, 0x11);
			tmp4 = _mm_shuffle_epi32(a, 78);
			tmp5 = _mm_shuffle_epi32(b, 78);
			tmp4 = _mm_xor_si128(tmp4, a);
			tmp5 = _mm_xor_si128(tmp5, b);
			tmp4 = _mm_clmulepi64_si128(tmp4, tmp5, 0x00);
			tmp4 = _mm_xor_si128(tmp4, tmp3);
			tmp4 = _mm_xor_si128(tmp4, tmp6);
			tmp5 = _mm_slli_si128(tmp4, 8);
			tmp4 = _mm_srli_si128(tmp4, 8);
			tmp3 = _mm_xor_si128(tmp3, tmp5);
			tmp6 = _mm_xor_si128(tmp6, tmp4);
			tmp7 = _mm_srli_epi32(tmp6, 31);
			tmp8 = _mm_srli_epi32(tmp6, 30);
			tmp9 = _mm_srli_epi32(tmp6, 25);
			tmp7 = _mm_xor_si128(tmp7, tmp8);
			tmp7 = _mm_xor_si128(tmp7, tmp9);
			tmp8 = _mm_shuffle_epi32(tmp7, 147);

			tmp7 = _mm_and_si128(XMMMASK, tmp8);
			tmp8 = _mm_andnot_si128(XMMMASK, tmp8);
			tmp3 = _mm_xor_si128(tmp3, tmp8);
			tmp6 = _mm_xor_si128(tmp6, tmp7);
			tmp10 = _mm_slli_epi32(tmp6, 1);
			tmp3 = _mm_xor_si128(tmp3, tmp10);
			tmp11 = _mm_slli_epi32(tmp6, 2);
			tmp3 = _mm_xor_si128(tmp3, tmp11);
			tmp12 = _mm_slli_epi32(tmp6, 7);
			tmp3 = _mm_xor_si128(tmp3, tmp12);

			*res = _mm_xor_si128(tmp3, tmp6);
		}
#else
		void gfmul(__m128i a, __m128i b, __m128i *res)
		{
			TODO("Implement it.");
		}
#endif

// this function works correctly only if all the upper half of b is zeros
#ifdef ENABLE_SSE
		void gfmulHalfZeros(__m128i a, __m128i b, __m128i *res)
		{
			__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6,
				tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
			__m128i XMMMASK = _mm_setr_epi32(0xffffffff, 0x0, 0x0, 0x0);
			tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
			tmp4 = _mm_shuffle_epi32(a, 78);
			tmp5 = _mm_shuffle_epi32(b, 78);
			tmp4 = _mm_xor_si128(tmp4, a);
			tmp5 = _mm_xor_si128(tmp5, b);
			tmp4 = _mm_clmulepi64_si128(tmp4, tmp5, 0x00);
			tmp4 = _mm_xor_si128(tmp4, tmp3);
			tmp5 = _mm_slli_si128(tmp4, 8);
			tmp4 = _mm_srli_si128(tmp4, 8);
			tmp3 = _mm_xor_si128(tmp3, tmp5);
			tmp6 = tmp4;
			tmp7 = _mm_srli_epi32(tmp6, 31);
			tmp8 = _mm_srli_epi32(tmp6, 30);
			tmp9 = _mm_srli_epi32(tmp6, 25);
			tmp7 = _mm_xor_si128(tmp7, tmp8);
			tmp7 = _mm_xor_si128(tmp7, tmp9);
			tmp8 = _mm_shuffle_epi32(tmp7, 147);

			tmp8 = _mm_andnot_si128(XMMMASK, tmp8);
			tmp3 = _mm_xor_si128(tmp3, tmp8);
			tmp10 = _mm_slli_epi32(tmp6, 1);
			tmp3 = _mm_xor_si128(tmp3, tmp10);
			tmp11 = _mm_slli_epi32(tmp6, 2);
			tmp3 = _mm_xor_si128(tmp3, tmp11);
			tmp12 = _mm_slli_epi32(tmp6, 7);
			tmp3 = _mm_xor_si128(tmp3, tmp12);
			*res = _mm_xor_si128(tmp3, tmp6);
		}
#else
		void gfmulHalfZeros(__m128i a, __m128i b, __m128i *res)
		{
			TODO("Implement it.");
		}
#endif

		// multiplies a and b
		__m128i gfmul(__m128i a, __m128i b)
		{
			__m128i ans;
			gfmul(a, b, &ans);
			return ans;
		}

		// This function works correctly only if all the upper half of b is zeros
		__m128i gfmulHalfZeros(__m128i a, __m128i b)
		{
			__m128i ans;
			gfmulHalfZeros(a, b, &ans);
			return ans;
		}

		__m128i gfpow(__m128i x, int deg)
		{
			__m128i ans = ONE;
			// TODO: improve this
			for (int i = 0; i < deg; i++)
				ans = MUL(ans, x);

			return ans;
		}

		__m128i fastgfpow(__m128i x, int deg)
		{
			__m128i ans = ONE;
			if (deg == 0)
				return ans;
			else if (deg == 1)
				return x;
			else
			{
				int hdeg = deg / 2;
				__m128i temp1 = fastgfpow(x, hdeg);
				__m128i temp2 = fastgfpow(x, deg - hdeg);
				ans = MUL(temp1, temp2);
			}
			return ans;
		}

		__m128i square(__m128i x)
		{
			return POW(x, 2);
		}

		__m128i inverse(__m128i x)
		{
			__m128i powers[128];
			powers[0] = x;
			__m128i ans = ONE;
			for (int i = 1; i < 128; i++)
			{
				powers[i] = SQ(powers[i - 1]);
				ans = MUL(ans, powers[i]);
			}
			return ans;
		}

		string _sha256hash_(char *input, int length)
		{
			unsigned char hash[SHA256_DIGEST_LENGTH];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, input, length);
			SHA256_Final(hash, &sha256);
			string output;
			output.resize(SHA256_DIGEST_LENGTH);
			for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
				output[i] = hash[i];
			return output;
		}

		string sha256hash(char *input, int length)
		{
			unsigned char hash[SHA256_DIGEST_LENGTH];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, input, length);
			SHA256_Final(hash, &sha256);
			char outputBuffer[65];
			for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
			{
				sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
			}
			outputBuffer[64] = 0;
			return outputBuffer;
		}

		void printError(string error)
		{
			cout << error << endl;
			exit(-1);
		}

		string __m128i_toHex(__m128i var)
		{
			static char const alphabet[] = "0123456789abcdef";
			// 	output += alphabet[((int)s[i]+128)/16];
			// 	output += alphabet[((int)s[i]+128)%16];
			// }

			char *val = (char *)&var;
			stringstream ss;
			for (unsigned int i = 0; i < 16; i++)
			{
				// if ((int)val[i] + 128 < 16)
				// ss << '0';
				// ss << std::hex << (int)val[i] + 128;

				ss << alphabet[((int)val[i] + 128) / 16];
				ss << alphabet[((int)val[i] + 128) % 16];
			}
			return ss.str();
		}

		string toHex(string s)
		{
			static char const alphabet[] = "0123456789abcdef";
			string output;
			for (int i = 0; i < s.length(); i++)
			{
				// cout << (int) s[i] << endl;
				// cout << ((int)s[i] & 0x80) << " " << ((int)s[i] & 0x40) << " " << ((int)s[i] & 0x20) << " " << ((int)s[i] & 0x10) << " " << ((int)s[i] & 0x08) << " " << ((int)s[i] & 0x04) << " " << ((int)s[i] & 0x02) << " " << ((int)s[i] & 0x01) << endl;
				output += alphabet[((int)s[i] + 128) / 16];
				output += alphabet[((int)s[i] + 128) % 16];
			}
			return output;
		}

		string __m128i_toString(__m128i var)
		{
			char *val = (char *)&var;
			stringstream ss;
			for (unsigned int i = 0; i < 16; i++)
				ss << val[i];
			return ss.str();
		}

		__m128i stringTo__m128i(string str)
		{
			if (str.length() != 16)
				cout << "Error: Length of input to stringTo__m128i is " << str.length() << endl;

			__m128i output;
			char *val = (char *)&output;
			for (int i = 0; i < 16; i++)
				val[i] = str[i];
			// cout << " converted " << std::flush;
			return output;
		}

		unsigned int charValue(char c)
		{
			if (c >= '0' && c <= '9')
			{
				return c - '0';
			}
			if (c >= 'a' && c <= 'f')
			{
				return c - 'a' + 10;
			}
			if (c >= 'A' && c <= 'F')
			{
				return c - 'A' + 10;
			}
			return -1;
		}

		string convertBooltoChars(bool *input, int length)
		{
			stringstream output;
			int count = 0;
			for (int i = 0; i < ceil((float)length / 8); i++)
			{
				int ch = 0;
				for (int c = 0; c < 8; c++)
				{
					ch *= 2;
					if (count < length)
						ch += input[count++];
				}
				output << (char)ch;
			}
			return output.str();
		}

		string convertCharsToString(char *input, int size)
		{
			stringstream ss;
			for (int i = 0; i < size; i++)
				ss << input[i];
			return ss.str();
		}

		void print(__m128i *arr, int size)
		{
			for (int i = 0; i < size; i++)
			{
				print128_num(arr[i]);
			}
		}

		void print128_num(__m128i var)
		{
			uint16_t *val = (uint16_t *)&var;
			printf("Numerical:%i %i %i %i %i %i %i %i\n",
				   val[0], val[1], val[2], val[3], val[4], val[5],
				   val[6], val[7]);
		}

		void log_print(string str)
		{
#if (LOG_DEBUG)
			cout << "----------------------------" << endl;
			cout << "Started " << str << endl;
			cout << "----------------------------" << endl;
#endif
		}

		void error(string str)
		{
			cout << "Error: " << str << endl;
			exit(-1);
		}

		string which_network(string network)
		{
			if (network.find("SecureML") != std::string::npos)
				return "SecureML";
			else if (network.find("MiniONN") != std::string::npos)
				return "MiniONN";
			else if (network.find("Sarda") != std::string::npos)
				return "Sarda";
			else if (network.find("LeNet") != std::string::npos)
				return "LeNet";
			else if (network.find("AlexNet") != std::string::npos)
				return "AlexNet";
			else if (network.find("VGG16") != std::string::npos)
				return "VGG16";
			else
				error("network not recognised in which_network");
		}

		void print_myType(myType var, string message, string type)
		{
			if (BIT_SIZE == 64)
			{
				if (type == "BITS")
					cout << message << ": " << bitset<BIT_SIZE>(var) << endl;
				else if (type == "FLOAT")
					cout << message << ": " << (static_cast<int64_t>(var)) / (float)(1 << FLOAT_PRECISION) << endl;
				else if (type == "SIGNED")
					cout << message << ": " << static_cast<int64_t>(var) << endl;
				else if (type == "UNSIGNED")
					cout << message << ": " << var << endl;
			}
			if (BIT_SIZE == 32)
			{
				if (type == "BITS")
					cout << message << ": " << bitset<BIT_SIZE>(var) << endl;
				else if (type == "FLOAT")
					cout << message << ": " << (static_cast<int32_t>(var)) / (float)(1 << FLOAT_PRECISION) << endl;
				else if (type == "SIGNED")
					cout << message << ": " << static_cast<int32_t>(var) << endl;
				else if (type == "UNSIGNED")
					cout << message << ": " << var << endl;
			}
		}

		void print_linear(myType var, string type)
		{
			if (BIT_SIZE == 64)
			{
				if (type == "BITS")
					cout << bitset<BIT_SIZE>(var) << " ";
				else if (type == "FLOAT")
					cout << (static_cast<int64_t>(var)) / (float)(1 << FLOAT_PRECISION) << " ";
				else if (type == "SIGNED")
					cout << static_cast<int64_t>(var) << " ";
				else if (type == "UNSIGNED")
					cout << var << " ";
			}
			if (BIT_SIZE == 32)
			{
				if (type == "BITS")
					cout << bitset<BIT_SIZE>(var) << " ";
				else if (type == "FLOAT")
					cout << (static_cast<int32_t>(var)) / (float)(1 << FLOAT_PRECISION) << " ";
				else if (type == "SIGNED")
					cout << static_cast<int32_t>(var) << " ";
				else if (type == "UNSIGNED")
					cout << var << " ";
			}
		}

		extern void funcReconstruct(const RSSVectorMyType &a, vector<myType> &b, size_t size, string str, bool print);
		void print_vector(RSSVectorMyType &var, string type, string pre_text, int print_nos)
		{
			size_t temp_size = var.size();
			vector<myType> b(temp_size);
			funcReconstruct(var, b, temp_size, "anything", false);
			cout << pre_text << endl;
			for (int i = 0; i < print_nos; ++i)
			{
				print_linear(b[i], type);
				// if (i % 10 == 9)
				// std::cout << std::endl;
			}
			cout << endl;
		}

		extern void funcReconstruct(const RSSVectorSmallType &a, vector<smallType> &b, size_t size, string str, bool print);
		void print_vector(RSSVectorSmallType &var, string type, string pre_text, int print_nos)
		{
			size_t temp_size = var.size();
			vector<smallType> b(temp_size);
			funcReconstruct(var, b, temp_size, "anything", false);
			cout << pre_text << endl;
			for (int i = 0; i < print_nos; ++i)
			{
				cout << b[i] << " ";
				// if (i % 10 == 9)
				// std::cout << std::endl;
			}
			cout << endl;
		}

		void matrixMultRSS(const RSSVectorMyType &a, const RSSVectorMyType &b, vector<myType> &temp3,
						   size_t rows, size_t common_dim, size_t columns,
						   size_t transpose_a, size_t transpose_b)
		{
#if (!USING_EIGEN)
			/********************************* Triple For Loop *********************************/
			RSSVectorMyType triple_a(rows * common_dim), triple_b(common_dim * columns);

			for (size_t i = 0; i < rows; ++i)
			{
				for (size_t j = 0; j < common_dim; ++j)
				{
					if (transpose_a)
						triple_a[i * common_dim + j] = a[j * rows + i];
					else
						triple_a[i * common_dim + j] = a[i * common_dim + j];
				}
			}

			for (size_t i = 0; i < common_dim; ++i)
			{
				for (size_t j = 0; j < columns; ++j)
				{
					if (transpose_b)
						triple_b[i * columns + j] = b[j * common_dim + i];
					else
						triple_b[i * columns + j] = b[i * columns + j];
				}
			}

			for (int i = 0; i < rows; ++i)
			{
				for (int j = 0; j < columns; ++j)
				{
					temp3[i * columns + j] = 0;
					for (int k = 0; k < common_dim; ++k)
					{
						temp3[i * columns + j] += triple_a[i * common_dim + k].first * triple_b[k * columns + j].first +
												  triple_a[i * common_dim + k].first * triple_b[k * columns + j].second +
												  triple_a[i * common_dim + k].second * triple_b[k * columns + j].first;
					}
				}
			}
/********************************* Triple For Loop *********************************/
#endif
#if (USING_EIGEN)
			/********************************* WITH EIGEN Mat-Mul *********************************/
			eigenMatrix eigen_a(rows, common_dim), eigen_b(common_dim, columns), eigen_c(rows, columns);

			for (size_t i = 0; i < rows; ++i)
			{
				for (size_t j = 0; j < common_dim; ++j)
				{
					if (transpose_a)
					{
						eigen_a.m_share[0](i, j) = a[j * rows + i].first;
						eigen_a.m_share[1](i, j) = a[j * rows + i].second;
					}
					else
					{
						eigen_a.m_share[0](i, j) = a[i * common_dim + j].first;
						eigen_a.m_share[1](i, j) = a[i * common_dim + j].second;
					}
				}
			}

			for (size_t i = 0; i < common_dim; ++i)
			{
				for (size_t j = 0; j < columns; ++j)
				{
					if (transpose_b)
					{
						eigen_b.m_share[0](i, j) = b[j * common_dim + i].first;
						eigen_b.m_share[1](i, j) = b[j * common_dim + i].second;
					}
					else
					{
						eigen_b.m_share[0](i, j) = b[i * columns + j].first;
						eigen_b.m_share[1](i, j) = b[i * columns + j].second;
					}
				}
			}

			eigen_c = eigen_a * eigen_b;

			for (size_t i = 0; i < rows; ++i)
				for (size_t j = 0; j < columns; ++j)
					temp3[i * columns + j] = eigen_c.m_share[0](i, j);
/********************************* WITH EIGEN Mat-Mul *********************************/
#endif
		}

		myType dividePlain(myType a, int b)
		{
			assert((b != 0) && "Cannot divide by 0");

			if (BIT_SIZE == 32)
				return static_cast<myType>(static_cast<int32_t>(a) / static_cast<int32_t>(b));
			if (BIT_SIZE == 64)
				return static_cast<myType>(static_cast<int64_t>(a) / static_cast<int64_t>(b));
		}

		void dividePlain(vector<myType> &vec, int divisor)
		{
			assert((divisor != 0) && "Cannot divide by 0");

			if (BIT_SIZE == 32)
				for (int i = 0; i < vec.size(); ++i)
					vec[i] = (myType)((double)((int32_t)vec[i]) / (double)((int32_t)divisor));

			if (BIT_SIZE == 64)
				for (int i = 0; i < vec.size(); ++i)
					vec[i] = (myType)((double)((int64_t)vec[i]) / (double)((int64_t)divisor));
		}

		size_t nextParty(size_t party)
		{
			size_t ret;

			switch (party)
			{
			case PARTY_A:
				ret = PARTY_B;
				break;
			case PARTY_B:
				ret = PARTY_C;
				break;
			case PARTY_C:
				ret = PARTY_A;
				break;
			}
			return ret;
		}

		size_t prevParty(size_t party)
		{
			size_t ret;

			switch (party)
			{
			case PARTY_A:
				ret = PARTY_C;
				break;
			case PARTY_B:
				ret = PARTY_A;
				break;
			case PARTY_C:
				ret = PARTY_B;
				break;
			}
			return ret;
		}

		void wrapAround(const vector<myType> &a, const vector<myType> &b,
						vector<smallType> &c, size_t size)
		{
			for (size_t i = 0; i < size; ++i)
				c[i] = wrapAround(a[i], b[i]);
		}

		void wrap3(const RSSVectorMyType &a, const vector<myType> &b,
				   vector<smallType> &c, size_t size)
		{
			for (size_t i = 0; i < size; ++i)
				c[i] = wrap3(a[i].first, a[i].second, b[i]);
		}

		void multiplyByScalar(const RSSVectorMyType &a, size_t scalar, RSSVectorMyType &b)
		{
			size_t size = a.size();
			for (int i = 0; i < size; ++i)
			{
				b[i].first = a[i].first * scalar;
				b[i].second = a[i].second * scalar;
			}
		}

		// a is just a vector, b has to be a matrix of size rows*columns
		// such that b read columns wise is the same as a.
		//  void transposeVector(const RSSVectorMyType &a, RSSVectorMyType &b, size_t rows, size_t columns)
		//  {
		//  	size_t totalSize = rows*columns;

		// 	for (int i = 0; i < totalSize; ++i)
		// 		b[(i % rows)*columns + (i/rows)] = a[i];
		// }

		// a is zero padded into b (initialized to zeros) with parameters as passed:
		// imageWidth, imageHeight, padding, inputFilters, batchSize
		void zeroPad(const RSSVectorMyType &a, RSSVectorMyType &b,
					 size_t iw, size_t ih, size_t P, size_t Din, size_t B)
		{
			size_t size_B = (iw + 2 * P) * (ih + 2 * P) * Din;
			size_t size_Din = (iw + 2 * P) * (ih + 2 * P);
			size_t size_w = (iw + 2 * P);

			for (size_t i = 0; i < B; ++i)
				for (size_t j = 0; j < Din; ++j)
					for (size_t k = 0; k < ih; ++k)
						for (size_t l = 0; l < iw; ++l)
						{
							b[i * size_B + j * size_Din + (k + P) * size_w + l + P] = a[i * Din * iw * ih + j * iw * ih + k * iw + l];
						}
		}

		// //vec1 is reshaped for a "convolutional multiplication" with the following params:
		// //imageWidth, imageHeight, padding, inputFilters, stride, batchSize
		// //Here, imageWidth and imageHeight are those of the zeroPadded output.
		// void convToMult(const RSSVectorMyType &vec1, RSSVectorMyType &vec2,
		// 				size_t iw, size_t ih, size_t f, size_t Din, size_t S, size_t B)
		// {
		// 	size_t loc_input, loc_output;
		// 	for (size_t i = 0; i < B; ++i)
		// 		for (size_t j = 0; j < (ih+2*P)-f+1; j+=S)
		// 			for (size_t k = 0; k < (iw+2*P)-f+1; k+=S)
		// 			{
		// 				loc_output = (i*ow*oh + j*ow + k);
		// 				for (size_t l = 0; l < Din; ++l)
		// 				{
		// 					loc_input = i*(iw+2*P)*(ih+2*P)*Din + l*(iw+2*P)*(ih+2*P) + j*(iw+2*P) + k;
		// 					for (size_t a = 0; a < f; ++a)			//filter height
		// 						for (size_t b = 0; b < f; ++b)		//filter width
		// 							vec2[(l*f*f + a*f + b)*ow*oh*B + loc_output] = vec1[loc_input + a*(iw+2*P) + b];
		// 				}
		// 			}
		// }
	} //
}