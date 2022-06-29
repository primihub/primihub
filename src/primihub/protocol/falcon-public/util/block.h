#ifndef FALCON_BLOCK_H_
#define FALCON_BLOCK_H_

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#ifndef ENABLE_SSE
#include "src/primihub/common/defines.h"


  typedef struct
  {
    uint8_t d_[16];
  } __m128i;

#endif
#endif
