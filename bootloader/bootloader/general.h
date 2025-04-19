#ifndef GENERAL_H
#define GENERAL_H

#include "registers.h"

#ifdef __cplusplus
extern "C"
{
#endif

    VOID
    tlbflush(
        VOID* adr
    );

    VOID 
    mfence(
        VOID
    );

#ifdef __cplusplus
}
#endif


#endif // GENERAL_H

