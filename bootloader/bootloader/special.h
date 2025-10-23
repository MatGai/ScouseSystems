#ifndef SPECIAL_H
#define SPECIAL_H

#include "registers.h"

#ifdef __cplusplus
extern "C"
{
#endif

    unsigned long long
    __readeflags(
        void
    );

    void
    __writeeflags(
        unsigned long long
    );

    unsigned char
    __cpuidsupport(
        void
    );

#ifdef __cplusplus
}
#endif

//
// Function prototypes for assembly functions, as the function definitions are reserved in masm
//



#endif // SPECIAL_H

