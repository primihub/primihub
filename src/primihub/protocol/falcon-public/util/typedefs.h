#ifndef __TYPEDEFS_H__BY_SGCHOI
#define __TYPEDEFS_H__BY_SGCHOI

#include <time.h>

//#define OTTiming
//#define OTEXT_USE_GMP
#define OTEXT_USE_ECC
//#define VERIFY_OT
//#define OTEXT_USE_OPENSSL
#define NUMOTBLOCKS 32

	static int CEIL_LOG2(int bits)
	{
		int targetlevel = 0, bitstemp = bits;
		while (bitstemp >>= 1)
			++targetlevel;
		return targetlevel + ((1 << targetlevel) > bits);
	}

	static int FLOOR_LOG2(int bits)
	{
		int targetlevel = 0;
		while (bits >>= 1)
			++targetlevel;
		return targetlevel;
	}

	/*static double getMillies(timeval timestart, timeval timeend)
	{
		long time1 = (timestart.tv_sec * 1000000) + (timestart.tv_usec );
		long time2 = (timeend.tv_sec * 1000000) + (timeend.tv_usec );

		return (double)(time2-time1)/1000000;
	}*/

	typedef int BOOL;
	typedef long LONG;

	typedef unsigned char BYTE;
	typedef unsigned short USHORT;
	typedef unsigned int UINT;
	typedef unsigned long ULONG;
	typedef unsigned long long UINT_64T;

	typedef ULONG DWORD;
	typedef UINT_64T REGISTER_SIZE;

	typedef REGISTER_SIZE REGSIZE;
#define LOG2_REGISTER_SIZE CEIL_LOG2(sizeof(REGISTER_SIZE) << 3)

#define SHA1_BYTES 20
#define SHA1_BITS 160

#define AES_KEY_BITS 128
#define AES_KEY_BYTES 16
#define AES_BITS 128
#define AES_BYTES 16
#define LOG2_AES_BITS CEIL_LOG2(AES_BITS)

#define FILL_BYTES AES_BYTES
#define FILL_BITS AES_BITS

#define OT_WINDOW_SIZE (AES_BITS * 4)
#define OT_WINDOW_SIZE_BYTES (AES_BYTES * 4)
//*******************CHECK THIS - maybe should be enlarged
#define MAX_REPLY_BITS 65536 // at most 2^16 bits may be sent in one go

#define RETRY_CONNECT 1000
#define CONNECT_TIMEO_MILISEC 10000

#define SYMMETRIC_SECURITY_PARAMETER 80 // security parameter for Yao's garbled circuits
#define SSP SYMMETRIC_SECURITY_PARAMETER
#define BYTES_SSP SSP / 8

#define NUM_EXECS_NAOR_PINKAS SSP
#define NUM_EXECS_NAOR_PINKAS_BYTES NUM_EXECS_NAOR_PINKAS / 8

#define OTEXT_BLOCK_SIZE_BITS AES_BITS
#define OTEXT_BLOCK_SIZE_BYTES AES_BYTES

#define VECTOR_INTERNAL_SIZE 8

#ifdef OTEXT_USE_OPENSSL

#include <openssl/evp.h>
#define AES_KEY_CTX EVP_CIPHER_CTX
#define OTEXT_HASH_INIT(sha) SHA_Init(sha)
#define OTEXT_HASH_UPDATE(sha, buf, bufsize) SHA_Update(sha, buf, bufsize)
#define OTEXT_HASH_FINAL(sha, sha_buf) SHA_Final(sha_buf, sha)

	const BYTE ZERO_IV[AES_BYTES] = {0};
	static int otextaesencdummy;

#define OTEXT_AES_KEY_INIT(ctx, buf)                                    \
	{                                                                   \
		EVP_CIPHER_CTX_init(ctx);                                       \
		EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, buf, ZERO_IV); \
	}
#define OTEXT_AES_ENCRYPT(keyctx, outbuf, inbuf) EVP_EncryptUpdate(keyctx, outbuf, &otextaesencdummy, inbuf, AES_BYTES)

#else
// TODO:change to functions using AES-NI
//#define AESNI
//#ifndef AESNI
#define AES_KEY_CTX F_AES_KEY
#define OTEXT_HASH_INIT(sha) sha1_starts(sha)
#define OTEXT_HASH_UPDATE(sha, buf, bufsize) sha1_update(sha, buf, bufsize)
#define OTEXT_HASH_FINAL(sha, sha_buf) sha1_finish(sha, sha_buf)

#define OTEXT_AES_KEY_INIT(ctx, buf) private_AES_set_encrypt_key(buf, AES_KEY_BITS, ctx)
#define OTEXT_AES_ENCRYPT(keyctx, outbuf, inbuf) falcon_AES_encrypt(inbuf, outbuf, keyctx)
	//#else
	//#define AES_KEY_CTX AES_KEY_TED
	//#define OTEXT_HASH_INIT(sha) sha1_starts(sha)
	//#define OTEXT_HASH_UPDATE(sha, buf, bufsize) sha1_update(sha, buf, bufsize)
	//#define OTEXT_HASH_FINAL(sha, sha_buf) sha1_finish(sha, sha_buf)
	//
	// #define OTEXT_AES_KEY_INIT(ctx, buf) {cout<<"set key"<<endl;\
//	system("PAUSE");\
//AES_set_encrypt_key(buf, AES_KEY_BITS, ctx);}//{system("PAUSE");AES_set_encrypt_key(buf, AES_KEY_BITS, ctx);system("PAUSE");}
	//#define OTEXT_AES_ENCRYPT(keyctx, outbuf, inbuf) \
//{	\
//	__m128i in=_mm_loadu_si128((const __m128i*)inbuf);	\
//	__m128i out;	\
//	cout<<"encrypt"<<endl;\
//	system("PAUSE");\
//	AES_encryptC(&in,&out, keyctx);\
//	memcpy(outbuf,&out,sizeof(__m128));\
//}//{system("PAUSE");AES_encryptC((__m128i*)inbuf,(__m128i*) outbuf, keyctx);system("PAUSE");}//outbuf=	(BYTE*)&out;
	//#endif

#endif

#define SERVER_ID 0
#define CLIENT_ID 1

#define MAX_INT (~0)
#if (MAX_INT == 0xFFFFFFFF)
#define MACHINE_SIZE_32
#elif (MAX_INT == 0xFFFFFFFFFFFFFFFF)
#define MACHINE_SIZE_64
#else
#define MACHINE_SIZE_16
#endif

	template <class T>
	T rem(T a, T b)
	{
		return ((a) > 0) ? (a) % (b) : (a) % (b) + abs(b);
	}

#define FALSE 0
#define TRUE 1
#define ZERO_BYTE 0
#define MAX_BYTE 0xFF
#define MAX_UINT 0xFFFFFFFF

#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>

	typedef unsigned short USHORT;
	typedef int socklen_t;
#pragma comment(lib, "wsock32.lib")

#define SleepMiliSec(x) Sleep(x)

#else // WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/tcp.h>

	typedef int SOCKET;
#define INVALID_SOCKET -1

#define SleepMiliSec(x) usleep((x) << 10)
#endif // WIN32

#define CEIL_DIVIDE(x, y) ((((x) + (y)-1) / (y)))

#define PadToRegisterSize(x) (PadToMultiple(x, OTEXT_BLOCK_SIZE_BITS))
#define PadToMultiple(x, y) ((((x) + (y)-1) / (y)) * (y))

#include <cstring>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

#endif //__TYPEDEFS_H__BY_SGCHOI

