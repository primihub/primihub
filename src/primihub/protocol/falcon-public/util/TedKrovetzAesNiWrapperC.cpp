#include "TedKrovetzAesNiWrapperC.h"
#include <iostream>
#include <cstring>
using namespace std;

__m128i seed[2];

__m128i* temp;

void (*myAES) (block *in, block *out, int nblks, AES_KEY_TED *aesKey);

AES_KEY_TED fixed_aes_key;


void AES_128_Key_Expansion(const unsigned char *userkey, AES_KEY_TED *aesKey)
{
    block x0,x1,x2;
    //block *kp = (block *)&aesKey;
	aesKey->rd_key[0] = x0 = _mm_loadu_si128((block*)userkey);
    x2 = _mm_setzero_si128();
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 1);   aesKey->rd_key[1] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 2);   aesKey->rd_key[2] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 4);   aesKey->rd_key[3] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 8);   aesKey->rd_key[4] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 16);  aesKey->rd_key[5] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 32);  aesKey->rd_key[6] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 64);  aesKey->rd_key[7] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 128); aesKey->rd_key[8] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 27);  aesKey->rd_key[9] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 54);  aesKey->rd_key[10] = x0;
}



void AES_192_Key_Expansion(const unsigned char *userkey, AES_KEY_TED *aesKey)
{
    __m128i x0,x1,x2,x3,tmp,*kp = (block *)&aesKey;
    kp[0] = x0 = _mm_loadu_si128((block*)userkey);
    tmp = x3 = _mm_loadu_si128((block*)(userkey+16));
    x2 = _mm_setzero_si128();
    EXPAND192_STEP(1,1);
    EXPAND192_STEP(4,4);
    EXPAND192_STEP(7,16);
    EXPAND192_STEP(10,64);
}

void AES_256_Key_Expansion(const unsigned char *userkey, AES_KEY_TED *aesKey)
{
	__m128i x0, x1, x2, x3;/* , *kp = (block *)&aesKey;*/
	aesKey->rd_key[0] = x0 = _mm_loadu_si128((block*)userkey);
	aesKey->rd_key[1] = x3 = _mm_loadu_si128((block*)(userkey + 16));
    x2 = _mm_setzero_si128();
    EXPAND_ASSIST(x0, x1, x2, x3, 255, 1);  aesKey->rd_key[2] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 1);  aesKey->rd_key[3] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 2);  aesKey->rd_key[4] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 2);  aesKey->rd_key[5] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 4);  aesKey->rd_key[6] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 4);  aesKey->rd_key[7] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 8);  aesKey->rd_key[8] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 8);  aesKey->rd_key[9] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 16); aesKey->rd_key[10] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 16); aesKey->rd_key[11] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 32); aesKey->rd_key[12] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 32); aesKey->rd_key[13] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 64); aesKey->rd_key[14] = x0;
}

void AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY_TED *aesKey)
{
    if (bits == 128) {
		AES_128_Key_Expansion(userKey, aesKey);
    } else if (bits == 192) {
		AES_192_Key_Expansion(userKey, aesKey);
    } else if (bits == 256) {
		AES_256_Key_Expansion(userKey, aesKey);
    }

	aesKey->rounds = 6 + bits / 32;
   
}

void AES_encryptC(block *in, block *out,  AES_KEY_TED *aesKey)
{
	int j, rnds = ROUNDS(aesKey);
	const __m128i *sched = ((__m128i *)(aesKey->rd_key));
	__m128i tmp = _mm_loadu_si128((__m128i*)in);
	tmp = _mm_xor_si128(tmp, sched[0]);
	for (j = 1; j<rnds; j++)  tmp = _mm_aesenc_si128(tmp, sched[j]);
	tmp = _mm_aesenclast_si128(tmp, sched[j]);
	_mm_storeu_si128((__m128i*)out, tmp);
}


void AES_ecb_encrypt(block *blk,  AES_KEY_TED *aesKey) {
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));

	*blk = _mm_xor_si128(*blk, sched[0]);
	for (j = 1; j<rnds; ++j)
		*blk = _mm_aesenc_si128(*blk, sched[j]);
	*blk = _mm_aesenclast_si128(*blk, sched[j]);
}

void AES_ecb_encrypt_blks(block *blks, unsigned nblks,  AES_KEY_TED *aesKey) {
    unsigned i,j,rnds=ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	for (i=0; i<nblks; ++i)
	    blks[i] =_mm_xor_si128(blks[i], sched[0]);
	for(j=1; j<rnds; ++j)
	    for (i=0; i<nblks; ++i)
		    blks[i] = _mm_aesenc_si128(blks[i], sched[j]);
	for (i=0; i<nblks; ++i)
	    blks[i] =_mm_aesenclast_si128(blks[i], sched[j]);
}

void AES_ecb_encrypt_blks_4(block *blks,  AES_KEY_TED *aesKey) {
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	blks[0] = _mm_xor_si128(blks[0], sched[0]);
	blks[1] = _mm_xor_si128(blks[1], sched[0]);
	blks[2] = _mm_xor_si128(blks[2], sched[0]);
	blks[3] = _mm_xor_si128(blks[3], sched[0]);

	for (j = 1; j < rnds; ++j){
		blks[0] = _mm_aesenc_si128(blks[0], sched[j]);
		blks[1] = _mm_aesenc_si128(blks[1], sched[j]);
		blks[2] = _mm_aesenc_si128(blks[2], sched[j]);
		blks[3] = _mm_aesenc_si128(blks[3], sched[j]);
	}
	blks[0] = _mm_aesenclast_si128(blks[0], sched[j]);
	blks[1] = _mm_aesenclast_si128(blks[1], sched[j]);
	blks[2] = _mm_aesenclast_si128(blks[2], sched[j]);
	blks[3] = _mm_aesenclast_si128(blks[3], sched[j]);
}

void AES_ecb_encrypt_blks_4_in_out(block *in, block *out,  AES_KEY_TED *aesKey) {
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	//block temp[4];

	out[0] = _mm_xor_si128(in[0], sched[0]);
	out[1] = _mm_xor_si128(in[1], sched[0]);
	out[2] = _mm_xor_si128(in[2], sched[0]);
	out[3] = _mm_xor_si128(in[3], sched[0]);

	for (j = 1; j < rnds; ++j){
		out[0] = _mm_aesenc_si128(out[0], sched[j]);
		out[1] = _mm_aesenc_si128(out[1], sched[j]);
		out[2] = _mm_aesenc_si128(out[2], sched[j]);
		out[3] = _mm_aesenc_si128(out[3], sched[j]);
	}
	out[0] = _mm_aesenclast_si128(out[0], sched[j]);
	out[1] = _mm_aesenclast_si128(out[1], sched[j]);
	out[2] = _mm_aesenclast_si128(out[2], sched[j]);
	out[3] = _mm_aesenclast_si128(out[3], sched[j]);
}

void AES_ecb_encrypt_chunk_in_out(block *in, block *out, unsigned nblks, AES_KEY_TED *aesKey) {

	int numberOfLoops = nblks / 8;
	int blocksPipeLined = numberOfLoops * 8;
	int remainingEncrypts = nblks - blocksPipeLined;

	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	for (int i = 0; i < numberOfLoops; i++){
		out[0 + i * 8] = _mm_xor_si128(in[0 + i * 8], sched[0]);
		out[1 + i * 8] = _mm_xor_si128(in[1 + i * 8], sched[0]);
		out[2 + i * 8] = _mm_xor_si128(in[2 + i * 8], sched[0]);
		out[3 + i * 8] = _mm_xor_si128(in[3 + i * 8], sched[0]);
		out[4 + i * 8] = _mm_xor_si128(in[4 + i * 8], sched[0]);
		out[5 + i * 8] = _mm_xor_si128(in[5 + i * 8], sched[0]);
		out[6 + i * 8] = _mm_xor_si128(in[6 + i * 8], sched[0]);
		out[7 + i * 8] = _mm_xor_si128(in[7 + i * 8], sched[0]);

		for (j = 1; j < rnds; ++j){
			out[0 + i * 8] = _mm_aesenc_si128(out[0 + i * 8], sched[j]);
			out[1 + i * 8] = _mm_aesenc_si128(out[1 + i * 8], sched[j]);
			out[2 + i * 8] = _mm_aesenc_si128(out[2 + i * 8], sched[j]);
			out[3 + i * 8] = _mm_aesenc_si128(out[3 + i * 8], sched[j]);
			out[4 + i * 8] = _mm_aesenc_si128(out[4 + i * 8], sched[j]);
			out[5 + i * 8] = _mm_aesenc_si128(out[5 + i * 8], sched[j]);
			out[6 + i * 8] = _mm_aesenc_si128(out[6 + i * 8], sched[j]);
			out[7 + i * 8] = _mm_aesenc_si128(out[7 + i * 8], sched[j]);
		}
		out[0 + i * 8] = _mm_aesenclast_si128(out[0 + i * 8], sched[j]);
		out[1 + i * 8] = _mm_aesenclast_si128(out[1 + i * 8], sched[j]);
		out[2 + i * 8] = _mm_aesenclast_si128(out[2 + i * 8], sched[j]);
		out[3 + i * 8] = _mm_aesenclast_si128(out[3 + i * 8], sched[j]);
		out[4 + i * 8] = _mm_aesenclast_si128(out[4 + i * 8], sched[j]);
		out[5 + i * 8] = _mm_aesenclast_si128(out[5 + i * 8], sched[j]);
		out[6 + i * 8] = _mm_aesenclast_si128(out[6 + i * 8], sched[j]);
		out[7 + i * 8] = _mm_aesenclast_si128(out[7 + i * 8], sched[j]);
	}
	for (int i = blocksPipeLined; i<blocksPipeLined+remainingEncrypts; ++i)
		out[i] = _mm_xor_si128(in[i], sched[0]);
	for (j = 1; j<rnds; ++j)
		for (int i = blocksPipeLined; i<blocksPipeLined+remainingEncrypts; ++i)
			out[i] = _mm_aesenc_si128(out[i], sched[j]);
	for (int i = blocksPipeLined; i<blocksPipeLined+remainingEncrypts; ++i)
		out[i] = _mm_aesenclast_si128(out[i], sched[j]);
}


void AES_ctr_hash_gate(block *in, block *out, int gate, int numOfParties, AES_KEY_TED *aesKey)
{
	__m128i counter,temp;
	for (int i = 0; i < numOfParties; i++)
	{
		temp = _mm_set1_epi32(gate*numOfParties + i);
		AES_encryptC(&temp, &counter, aesKey);
		out[i] = _mm_xor_si128(in[i],temp);
	}
}

void AES_ctr_hash_gate(block *in, block *out, int gate, int numOfParties, __m128i seedIn1,__m128i seedIn2)
{
	__m128i counter, temp = { 0 };
	__m128i* seed=static_cast<__m128i *>(_aligned_malloc(2*sizeof(__m128i), 16));;
	seed[0] = seedIn1;
	seed[1] = seedIn2;
	AES_KEY_TED aes_key;
	AES_set_encrypt_key((unsigned char*)seed, 256, &aes_key);
	for (int i = 0; i < numOfParties; i++)
	{
		temp = _mm_set1_epi32(gate*numOfParties + i);
		AES_encryptC(&temp, &counter, &aes_key);
		out[i] = _mm_xor_si128(in[i], temp);
	}
}

__m128i* pseudoRandomFunction(__m128i seed1, __m128i seed2, int gate, int numOfParties)
{
	__m128i counter = { 0 }, temp;// = { 0 };
	__m128i* result = static_cast<__m128i *>(_aligned_malloc(numOfParties * sizeof(__m128i), 16));
	seed[0] = seed1;
	seed[1] = seed2;
	AES_KEY_TED aes_key;
	AES_set_encrypt_key((unsigned char*)seed, 256, &aes_key);
	int init = gate*numOfParties;
	for (int i = 0; i < numOfParties; i++)
	{
		counter = _mm_set1_epi32(init + i);
		AES_encryptC(&counter, &temp, &aes_key);
		result[i] = temp;
	}
	return result;
}

void pseudoRandomFunctionNew(__m128i seed1, __m128i seed2, int gate, int numOfParties, __m128i* out)
{
	__m128i counter = { 0 }, temp;// = { 0 };
	
	seed[0] = seed1;
	seed[1] = seed2;
	AES_KEY_TED aes_key;
	AES_set_encrypt_key((unsigned char*)seed, 256, &aes_key);
	int init = gate*numOfParties;
	for (int i = 0; i < numOfParties; i++)
	{
		counter = _mm_set1_epi32(init + i);
		AES_encryptC(&counter, &temp, &aes_key);
		out[i] = temp;
	}
}

void AES_init(int numOfParties)
{
	//temp = static_cast<__m128i *>(_aligned_malloc(numOfParties * sizeof(__m128i), 16));
	switch(numOfParties)
	{
	    case 3:
		myAES = &AES_ecb_encrypt_for_3;
		break;
	    case 4:
		myAES = &AES_ecb_encrypt_for_4;
		break;
	    case 5:
		myAES = &AES_ecb_encrypt_for_5;
		break;
	    case 7:
		myAES = &AES_ecb_encrypt_for_7;
		break;
	    //numOfParties>7 - use regular function
	    default:
		myAES = reinterpret_cast<decltype(myAES)>(&AES_ecb_encrypt_chunk_in_out);
	}
}

void AES_free()
{
	_aligned_free(temp);
}

bool firstBit(__m128i input)
{
	char *inp = (char *)&input;
	return (input[0] & 0x80 > 0);
}

bool pseudoRandomFunctionwPipelining(__m128i seed1, __m128i seed2, int gate, int numOfParties, __m128i* out)
{
	__m128i counter = {0};
	
	seed[0] = seed1;
	seed[1] = seed2;
	AES_KEY_TED aes_key;
	AES_set_encrypt_key((unsigned char*)seed, 256, &aes_key);
	__m128i *temp_in = static_cast<__m128i*>(_aligned_malloc(sizeof(__m128i)*(numOfParties+1), 16));
	__m128i *temp_out = static_cast<__m128i*>(_aligned_malloc(sizeof(__m128i)*(numOfParties+1), 16));
	for (int i = 0; i < numOfParties+1; i++)
	{
		temp_in[i] = _mm_set_epi32(0,0,gate,i);
	}
	if (numOfParties == 4) AES_ecb_encrypt_for_5(temp_in, temp_out, numOfParties+1, &aes_key);
	else AES_ecb_encrypt_chunk_in_out(temp_in, temp_out, numOfParties+1, &aes_key);
	memcpy(out, temp_out, numOfParties*sizeof(__m128i));	 //-----------------------Check if order of arguments is correct
	bool output = firstBit(temp_out[numOfParties]);
	_aligned_free(temp_in);
	_aligned_free(temp_out);
	return output;
	// return 0;
	// return firstBit(seed1);
}

//XORs 2 vectors
void XORvectors2(__m128i *vec1, __m128i *vec2, __m128i *out, int length)
{
	for (int p = 0; p < length; p++)
	{
		out[p] = _mm_xor_si128(vec1[p], vec2[p]);
	}
}

bool fixedKeyPseudoRandomFunctionwPipelining(__m128i seed1, __m128i seed2, int gate, int numOfParties, __m128i* out)
{
	__m128i counter = {0};
	__m128i dseed1 = _mm_slli_epi64(seed1, 1);
	__m128i ddseed2 = _mm_slli_epi64(seed2, 2);
	__m128i *temp_in = static_cast<__m128i*>(_aligned_malloc(sizeof(__m128i)*(numOfParties+1), 16));
	__m128i *temp_out = static_cast<__m128i*>(_aligned_malloc(sizeof(__m128i)*(numOfParties+1), 16));
	for (int i = 0; i < numOfParties+1; i++)
	{
		temp_in[i] = _mm_xor_si128(_mm_xor_si128(_mm_set_epi32(0,0,gate,i),dseed1),ddseed2);
	}
	if (numOfParties == 4) AES_ecb_encrypt_for_5(temp_in, temp_out, numOfParties+1, &fixed_aes_key);
	else AES_ecb_encrypt_chunk_in_out(temp_in, temp_out, numOfParties+1, &fixed_aes_key);
	XORvectors2(temp_out, temp_in, out, numOfParties);
	bool output = firstBit(temp_out[numOfParties]);
	_aligned_free(temp_in);
	_aligned_free(temp_out);
	return output;
	// return 0;
}



//assumes nblks==7
void AES_ecb_encrypt_for_7(block *in, block *out, int nblks,  AES_KEY_TED *aesKey)
{
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	//block temp[4];
	out[0 ] = _mm_xor_si128(in[0 ], sched[0]);
	out[1 ] = _mm_xor_si128(in[1 ], sched[0]);
	out[2 ] = _mm_xor_si128(in[2 ], sched[0]);
	out[3 ] = _mm_xor_si128(in[3 ], sched[0]);
	out[4 ] = _mm_xor_si128(in[4 ], sched[0]);
	out[5 ] = _mm_xor_si128(in[5 ], sched[0]);
	out[6 ] = _mm_xor_si128(in[6 ], sched[0]);
	
	for (j = 1; j < rnds; ++j){
	  out[0 ] = _mm_aesenc_si128(out[0 ], sched[j]);
	  out[1 ] = _mm_aesenc_si128(out[1 ], sched[j]);
	  out[2 ] = _mm_aesenc_si128(out[2 ], sched[j]);
	  out[3 ] = _mm_aesenc_si128(out[3 ], sched[j]);
	  out[4 ] = _mm_aesenc_si128(out[4 ], sched[j]);
	  out[5 ] = _mm_aesenc_si128(out[5 ], sched[j]);
	  out[6 ] = _mm_aesenc_si128(out[6 ], sched[j]);
	}
	out[0 ] = _mm_aesenclast_si128(out[0 ], sched[j]);
	out[1 ] = _mm_aesenclast_si128(out[1 ], sched[j]);
	out[2 ] = _mm_aesenclast_si128(out[2 ], sched[j]);
	out[3 ] = _mm_aesenclast_si128(out[3 ], sched[j]);
	out[4 ] = _mm_aesenclast_si128(out[4 ], sched[j]);
	out[5 ] = _mm_aesenclast_si128(out[5 ], sched[j]);
	out[6 ] = _mm_aesenclast_si128(out[6 ], sched[j]);
}

//assumes nblks==5
void AES_ecb_encrypt_for_5(block *in, block *out, int nblks,  AES_KEY_TED *aesKey)
{
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	//block temp[4];
	out[0 ] = _mm_xor_si128(in[0 ], sched[0]);
	out[1 ] = _mm_xor_si128(in[1 ], sched[0]);
	out[2 ] = _mm_xor_si128(in[2 ], sched[0]);
	out[3 ] = _mm_xor_si128(in[3 ], sched[0]);
	out[4 ] = _mm_xor_si128(in[4 ], sched[0]);
	
	for (j = 1; j < rnds; ++j){
	  out[0 ] = _mm_aesenc_si128(out[0 ], sched[j]);
	  out[1 ] = _mm_aesenc_si128(out[1 ], sched[j]);
	  out[2 ] = _mm_aesenc_si128(out[2 ], sched[j]);
	  out[3 ] = _mm_aesenc_si128(out[3 ], sched[j]);
	  out[4 ] = _mm_aesenc_si128(out[4 ], sched[j]);
	}
	
	out[0 ] = _mm_aesenclast_si128(out[0 ], sched[j]);
	out[1 ] = _mm_aesenclast_si128(out[1 ], sched[j]);
	out[2 ] = _mm_aesenclast_si128(out[2 ], sched[j]);
	out[3 ] = _mm_aesenclast_si128(out[3 ], sched[j]);
	out[4 ] = _mm_aesenclast_si128(out[4 ], sched[j]);
}

//assumes nblks==3
void AES_ecb_encrypt_for_3(block *in, block *out, int nblks,  AES_KEY_TED *aesKey)
{
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	//block temp[4];
	out[0 ] = _mm_xor_si128(in[0 ], sched[0]);
	out[1 ] = _mm_xor_si128(in[1 ], sched[0]);
	out[2 ] = _mm_xor_si128(in[2 ], sched[0]);
	
	for (j = 1; j < rnds; ++j){
	  out[0 ] = _mm_aesenc_si128(out[0 ], sched[j]);
	  out[1 ] = _mm_aesenc_si128(out[1 ], sched[j]);
	  out[2 ] = _mm_aesenc_si128(out[2 ], sched[j]);
	}
	
	out[0 ] = _mm_aesenclast_si128(out[0 ], sched[j]);
	out[1 ] = _mm_aesenclast_si128(out[1 ], sched[j]);
	out[2 ] = _mm_aesenclast_si128(out[2 ], sched[j]);
}

//assumes nblks==3
block AES_ecb_encrypt_for_1(block in, AES_KEY_TED *aesKey)
{
	unsigned j, rnds = ROUNDS(aesKey);
	block out;
	const block *sched = ((block *)(aesKey->rd_key));
	//block temp[4];
	out = _mm_xor_si128(in, sched[0]);
	
	for (j = 1; j < rnds; ++j){
	  out = _mm_aesenc_si128(out, sched[j]);
	}
	
	out = _mm_aesenclast_si128(out, sched[j]);
	return out;
}


void AES_ecb_encrypt_for_4(block *in, block *out, int nblks,  AES_KEY_TED *aesKey)
{
	unsigned j, rnds = ROUNDS(aesKey);
	const block *sched = ((block *)(aesKey->rd_key));
	//block temp[4];
	out[0 ] = _mm_xor_si128(in[0 ], sched[0]);
	out[1 ] = _mm_xor_si128(in[1 ], sched[0]);
	out[2 ] = _mm_xor_si128(in[2 ], sched[0]);
	out[3 ] = _mm_xor_si128(in[3 ], sched[0]);
	
	for (j = 1; j < rnds; ++j){
	  out[0 ] = _mm_aesenc_si128(out[0 ], sched[j]);
	  out[1 ] = _mm_aesenc_si128(out[1 ], sched[j]);
	  out[2 ] = _mm_aesenc_si128(out[2 ], sched[j]);
	  out[3 ] = _mm_aesenc_si128(out[3 ], sched[j]);
	}
	
	out[0 ] = _mm_aesenclast_si128(out[0 ], sched[j]);
	out[1 ] = _mm_aesenclast_si128(out[1 ], sched[j]);
	out[2 ] = _mm_aesenclast_si128(out[2 ], sched[j]);
	out[3 ] = _mm_aesenclast_si128(out[3 ], sched[j]);

}
