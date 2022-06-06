/*
 *   MIRACL compiler/hardware definitions - mirdef.h
 */

#define MR_LITTLE_ENDIAN
#define MIRACL 32
#define mr_utype int
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define MR_STRIPPED_DOWN
#define mr_dltype __int64
#define mr_unsign64 unsigned __int64
#define MR_GENERIC_MT
#define MR_NOASM
#define MR_COMBA 16
#define MR_BITSINCHAR 8
#define MAXBASE ((mr_small)1<<(MIRACL-1))

