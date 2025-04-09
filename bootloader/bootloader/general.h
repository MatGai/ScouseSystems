#ifndef GENERAL_H
#define GENERAL_H

#include "registers.h"

#ifdef __cplusplus
extern "C"
{
#endif

    VOID
    __tlbflush(
        VOID* adr
    );

    VOID 
    __mfence(
        VOID
    );

#ifdef __cplusplus
}
#endif


#endif // GENERAL_H

