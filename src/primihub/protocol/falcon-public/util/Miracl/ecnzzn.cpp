//
// Utility functions to force an ECn to be created from 2 or 3 ZZn
// And to extract an ECn into ZZns
//

#include "ecnzzn.h"

#ifndef MR_AFFINE_ONLY

void force(ZZn& x,ZZn& y,ZZn& z,ECn& A)
{  // A=(x,y,z)
    copy(getbig(x),A.get_point()->X);
    copy(getbig(y),A.get_point()->Y);
    copy(getbig(z),A.get_point()->Z);
    A.get_point()->marker=MR_EPOINT_GENERAL;
}

void extract(ECn &A, ZZn& x,ZZn& y,ZZn& z)
{ // (x,y,z) <- A
    big t;
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
    t=(A.get_point())->Z;
    if (A.get_status()!=MR_EPOINT_GENERAL) z=1;
    else                                   z=t;
}

#endif

void force(ZZn& x,ZZn& y,ECn& A)
{ // A=(x,y)
    copy(getbig(x),A.get_point()->X);
    copy(getbig(y),A.get_point()->Y);
    A.get_point()->marker=MR_EPOINT_NORMALIZED;
}

void extract(ECn& A,ZZn& x,ZZn& y)
{ // (x,y) <- A
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}
