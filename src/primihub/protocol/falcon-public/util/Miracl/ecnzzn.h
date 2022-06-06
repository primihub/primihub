//
// Utility functions to force an ECn to be created from 2 or 3 ZZn
// And to extract an ECn into ZZns
//

#ifndef ECNZZN_H
#define ECNZZN_H

#include "zzn.h"
#include "ecn.h"

#ifndef MR_AFFINE_ONLY

extern void force(ZZn&,ZZn&,ZZn&,ECn&);
extern void extract(ECn&,ZZn&,ZZn&,ZZn&);

#endif

extern void force(ZZn&,ZZn&,ECn&);
extern void extract(ECn&,ZZn&,ZZn&);

#endif
