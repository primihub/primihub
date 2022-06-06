/*
 * BitVector.h
 *
 *  Created on: May 6, 2013
 *      Author: mzohner
 */

#ifndef CBITVECTOR_H_
#define CBITVECTOR_H_

#ifdef OTEXT_USE_OPENSSL
#include <openssl/sha.h>
#else
#include "../util/sha1.h"
#endif
#include "../util/aes.h"
#include "../util/typedefs.h"
#include <math.h>
#include <iostream>
#include <iomanip>
#include "TedKrovetzAesNiWrapperC.h"//suspicious

struct SHA_BUFFER
{
	BYTE* data;
	operator BYTE* (){ return data; }
};

typedef SHA_BUFFER sha1_reg_context;

static const BYTE REVERSE_NIBBLE_ORDER[16] =
	{0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};

static const BYTE REVERSE_BYTE_ORDER[256] =
	{0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, \
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, \
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, \
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, \
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, \
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, \
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, \
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, \
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, \
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, \
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, \
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, \
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, \
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, \
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, \
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
	};

static const BYTE RESET_BIT_POSITIONS[9] =
	{0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

static const BYTE RESET_BIT_POSITIONS_INV[9] =
	{0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};

static const BYTE GET_BIT_POSITIONS[9] =
	{0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};

static const BYTE GET_BIT_POSITIONS_INV[9] =
	{0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00};

static const int INT_MASK[8] =
	{0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

static const int FIRST_MASK_SHIFT[8] =
	{0xFF00, 0x7F80, 0x3FC0, 0x1FE0, 0x0FF0, 0x07F8, 0x03FC, 0x01FE};

static const BYTE MASK_BIT[8] =
	{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

static const BYTE BIT[8] =
	{0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

static const BYTE CMASK_BIT[8] =
	{0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

static const BYTE C_BIT[8] =
	{0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

static const BYTE MASK_SET_BIT[2][8] =
	{{0,0,0,0,0,0,0,0},{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1}};

static const BYTE MASK_SET_BIT_C[2][8] =
	{{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1},{0,0,0,0,0,0,0,0}};

static const BYTE SET_BIT_C[2][8] =
	{{0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80},{0,0,0,0,0,0,0,0}};

const BYTE SELECT_BIT_POSITIONS[9] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};


#ifdef MACHINE_SIZE_32
static const REGISTER_SIZE TRANSPOSITION_MASKS[6] =
	{0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
static const REGISTER_SIZE TRANSPOSITION_MASKS_INV[6] =
	{0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
#else
	#ifdef MACHINE_SIZE_64
static const REGISTER_SIZE TRANSPOSITION_MASKS[6] =
	{0x5555555555555555, 0x3333333333333333, 0x0F0F0F0F0F0F0F0F, 0x00FF00FF00FF00FF, 0x0000FFFF0000FFFF, 0x00000000FFFFFFFF};
static const REGISTER_SIZE TRANSPOSITION_MASKS_INV[6] =
	{0xAAAAAAAAAAAAAAAA, 0xCCCCCCCCCCCCCCCC, 0xF0F0F0F0F0F0F0F0, 0xFF00FF00FF00FF00, 0xFFFF0000FFFF0000, 0xFFFFFFFF00000000};
	#else
	#endif
#endif

static const size_t SHIFTVAL = 3;//sizeof(REGSIZE);

class CBitVector
{
public:
	CBitVector(){ m_pBits =  NULL; m_nSize = 0; m_nKey.rounds = 0;}
	CBitVector(int bits){ m_pBits =  NULL; m_nSize = 0; m_nKey.rounds = 0; Create(bits);}
	CBitVector(int bits, BYTE* seed, int& cnt){ m_pBits =  NULL;
	m_nSize = 0; m_nKey.rounds = 0; Create(bits, seed, cnt);}

	//~CBitVector(){ if(m_pBits) free(m_pBits); }
	void delCBitVector(){ if(m_pBits) free(m_pBits); }

	//TODO: change to use AES-NI
	/* Use this function to initialize the AES round key which can then be used to generate further random bits */
	void InitRand(BYTE* seed) { OTEXT_AES_KEY_INIT(&m_nKey,seed); }//private_AES_set_encrypt_key(seed, AES_KEY_BITS, &m_nKey);}

	/* Fill the bitvector with random values and pre-initialize the key to the seed-key*/
	void FillRand(int bits, BYTE* seed, int& cnt);

	/* Fill random values using the pre-defined AES key */
	void FillRand(int bits, int& cnt);

	/**
	 *  Create Operations
	 */

	//Create in bits and bytes
	void Create(int bits);
	void CreateBytes(int bytes) {	Create(bytes<<3); }
	void CreateZeros(int bits) { Create(bits);		memset(m_pBits, 0, m_nSize); }
	//Create and fill with random values
	void Create(int bits, BYTE* seed, int& cnt);
	//Create an abstract array of values
	void Create(int numelements, int elementlength);
	void Create(int numelements, int elementlength, BYTE* seed, int& cnt);
	//Create an abstract two-dimensional array of values
	void Create(int numelementsDimA, int numelementsDimB, int elementlength);
	void Create(int numelementsDimA, int numelementsDimB, int elementlength, BYTE* seed, int& cnt);


	/*
	 * Management operations
	 */
	void ResizeinBytes(int newSizeBytes);
	void Reset() { memset(m_pBits, 0, m_nSize);	}
	void SetToOne() { memset(m_pBits, 0xFF, m_nSize);	}
	int GetSize(){ return m_nSize; }
	BOOL IsEqual(CBitVector& vec);
	void SetElementLength(int elelen) {	m_nElementLength = elelen; }


	/*
	 * Copy operations
	 */

	void Copy(CBitVector& vec) { Copy(vec.GetArr(), 0, vec.GetSize());	}
	void Copy(CBitVector& vec, int pos, int len) { Copy(vec.GetArr(), pos, len); }
	void Copy(BYTE* p, int pos, int len);



	void XOR_no_mask(int p, int bitPos, int bitLen);
	unsigned int GetInt(int bitPos, int bitLen);
	#define GetIntBitsFromLen(x, from, len) 	( ( (x & ( ( (2<<(len))-1) << from )) >> from) & 0xFF)
	#define GetMask(len) 				(( (1<<(len))-1))

	void ORByte(int pos, BYTE p);


	/*
	 * Bitwise operations
	 */
	BYTE GetBit(int idx) { return !!(m_pBits[idx>>3] & MASK_BIT[idx & 0x7]); }
	void SetBit(int idx, BYTE b) {	m_pBits[idx>>3] = (m_pBits[idx>>3] & CMASK_BIT[idx & 0x7]) | MASK_SET_BIT_C[!b][idx & 0x7];	}
	void XORBit(int idx, BYTE b) {	m_pBits[idx>>3] ^= MASK_SET_BIT_C[!b][idx & 0x7]; }
	void ANDBit(int idx, BYTE b) {	if(!b) m_pBits[idx>>3] &= CMASK_BIT[idx & 0x7]; }

	//used to access bits in the regular order
	BYTE GetBitNoMask(int idx) { return !!(m_pBits[idx>>3] & BIT[idx & 0x7]); }
	void SetBitNoMask(int idx, BYTE b) {	m_pBits[idx>>3] = (m_pBits[idx>>3] & C_BIT[idx & 0x7]) | SET_BIT_C[!b][idx & 0x7];	}
	void XORBitNoMask(int idx, BYTE b) {	m_pBits[idx>>3] ^= SET_BIT_C[!b][idx & 0x7]; }
	void ANDBitNoMask(int idx, BYTE b) {	if(!b) m_pBits[idx>>3] &= C_BIT[idx & 0x7]; }


	/*
	 * Single byte operations
	 */
	void SetByte(int idx, BYTE p) {	m_pBits[idx] = p;}
	BYTE GetByte(int idx) {	return m_pBits[idx]; }
	void XORByte(int idx, BYTE b) {	m_pBits[idx] ^= b; }
	void ANDByte(int idx, BYTE b) {	m_pBits[idx] &= b; }

	/*
	 * Get Operations
	 */
	void GetBits(BYTE* p, int pos, int len);
	void GetBytes(BYTE* p, int pos, int len);
	template <class T> void GetBytes(T* dst, T* src, T* lim);
	template <class T> T Get(int pos, int len)
	{
		T val = 0;
		GetBits((BYTE*) &val, pos, len);
		return val;
	}
	//NEW
	__m128i GetBlock(int pos){return _mm_set_epi32(GetInt(pos * 128 + 96, 32), GetInt(pos * 128 + 64, 32), GetInt(pos * 128 + 32, 32), GetInt(pos * 128, 32));}


	/*
	 * Set Operations
	 */
	void SetBits(BYTE* p, int pos, int len);
	void SetBytes(BYTE* p, int pos, int len);
	template <class T> void SetBytes(T* dst, T* src, T* lim);
	template <class T> void Set(T val, int pos, int len) { SetBits((BYTE*) &val, pos, len); }
	void SetBitsToZero(int bitpos, int bitlen);


	/*
	 * XOR Operations
	 */
	void XORBytes(BYTE* p, int pos, int len);
	void XORBytes(BYTE* p, int len) { XORBytes(p, 0, len); }
	void XORVector(CBitVector &vec, int pos, int len) { XORBytes(vec.GetArr(), pos, len); }
	template <class T> void XOR(T val, int pos, int len) { XORBits((BYTE*) &val, pos, len); }
	void XORBits(BYTE* p, int pos, int len);
	void XORBitsPosOffset(BYTE* p, int ppos, int pos, int len);
	template <class T> void XORBytes(T* dst, T* src, T* lim);
	void XORRepeat(BYTE* p, int pos, int len, int num);
	void XORBytesReverse(BYTE* p, int pos, int len);



	/*
	 * AND Operations
	 */
	void ANDBytes(BYTE* p, int pos, int len);
	template <class T> void ANDBytes(T* dst, T* src, T* lim);


	/*
	 * Set operations
	 */
	void SetXOR(BYTE* p, BYTE* q, int pos, int len);
	void SetAND(BYTE* p, BYTE* q, int pos, int len);

	/*
	 * Buffer access operations
	 */
	BYTE* GetArr(){ return m_pBits;}
	void AttachBuf(BYTE* p, int size=-1){ m_pBits = p; m_nSize = size;}
	void DetachBuf(){ m_pBits = NULL; m_nSize = 0;}


	/*
	 * Print Operations
	 */
	void Print(int fromBit, int toBit);
	void PrintHex();
	void PrintBinary() { Print(0, m_nSize<<3); }
	void PrintContent();


	/*
	 * If the cbitvector is abstracted to an array of elements with m_nElementLength bits size, these methods can be used for easier access
	 */
	template <class T> T Get(int i){ return Get<T>(i*m_nElementLength, m_nElementLength);}
	template <class T> void Set(int i, T val){ return Set<T>(val, i*m_nElementLength, m_nElementLength);}
	/*
	 * The same as the above methods only for two-dimensional access
	 */
	template <class T> T Get2D(int i, int j){ return Get<T>((i * m_nNumElementsDimB + j) * m_nElementLength, m_nElementLength);}
	template <class T> void Set2D(int i, int j, T val){ return Set<T>(val, (i * m_nNumElementsDimB + j) * m_nElementLength, m_nElementLength);}
	//useful when accessing elements using an index


	//View the cbitvector as a rows x columns matrix and transpose
	void EklundhBitTranspose(int rows, int columns);
	void SimpleTranspose(int rows, int columns);



private:
	BYTE*		m_pBits;
	int			m_nSize;
	AES_KEY_CTX 	m_nKey;
	int 		m_nBits; //The exact number of bits
	int			m_nElementLength;
	int 		m_nNumElements;
	int			m_nNumElementsDimB;
};




#endif /* BITVECTOR_H_ */
