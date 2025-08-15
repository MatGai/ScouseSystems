#ifndef GENERAL_H
#define GENERAL_H

#include "registers.h"

#ifdef __cplusplus
extern "C"
{
#endif

    VOID
    _tlbflush(
        VOID* adr
    );

    VOID 
    _mfence(
        VOID
    );

#ifdef __cplusplus
}
#endif

//
// Function prototypes for assembly functions, as the function definitions are reserved in masm
//

#define tlbflush(adr) _tlbflush((VOID*)(adr))
#define mfence() _mfence()

#endif // GENERAL_H

