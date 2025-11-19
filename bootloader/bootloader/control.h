#pragma once

#include "registers.h"


#ifdef __cplusplus
extern "C"
{
#endif

    ULONG64 __readcr0(
        VOID
    );

    VOID __writecr0(
        ULONG64 Value
    );

    ULONG64 __readcr3(
        VOID
    );

    VOID __writecr3(
        ULONG64 Value
    );

    ULONG64 __readcr4(
        VOID
    );

    VOID __writecr4(
        ULONG64 Value
    );


#ifdef __cplusplus
}
#endif

//
// CR0 
//

// enables protected mode
#define CR0_PROTECTED_MODE_ENABLE (1ull)

// enables monitoring of coprocessor
#define CR0_MONITOR_COPROCESSOR (1ull << 1)

// force all x87 and MMX instructions to cause a 'Device Not Found' exception
#define CR0_EMULATE_COPROCESSOR (1ull << 2)

// allows saving x87/MMX/SSE instructions on hardware context switches
#define CR0_TASK_SWITCHED (1ull << 3)

// checks for support of 387DX math coprocessor instruction
#define CR0_EXTENSION_TYPE (1ull << 4)

// enables internel error reporting mechanism for x87 FPU errors
#define CR0_NUMERIC_ERROR (1ull << 5)

// when set, it is not possible to write read-only pages
#define CR0_WRITE_PROTECT (1ull << 16)

// allows for checking alignment in usermode 
#define CR0_ALIGNMENT_MASK (1ull << 18)

// older cpus use this to control write-back/write-through cache strategy
#define CR0_NOT_WRITE_THROUGH (1ull << 29)

// disable some processor caches, specifics model-dependent
#define CR0_CACHE_DISABLE (1ull << 30)

// enables paging, PROTECTED MODE MUST BE ENABLED!
#define CR0_PAGING (1ull << 31)

typedef union _CR0_T
{
    ULONG64 Value; // raw cr0 value
    struct
    {
        ULONG64 PROTECTED_MODE_ENABLE : 1;  // bit 1
        ULONG64 MONITOR_COPROCESSOR : 1;    // bit 2
        ULONG64 EMULATE_COPROCESSOR : 1;    // bit 3
        ULONG64 TASK_SWITCHED : 1;          // bit 4
        ULONG64 EXTENSION_TYPE : 1;         // bit 5
        ULONG64 NUMERIC_ERROR : 1;          // bit 6
        ULONG64 __reserved1 : 9;            // bits 6 : 15
        ULONG64 WRITE_PROTECT : 1;          // bit 16
        ULONG64 __reserved2 : 1;            // bit 17
        ULONG64 ALIGNMENT_MASK : 1;         // bit 18
        ULONG64 __reserved3 : 10;           // bits 19 : 28
        ULONG64 NOT_WRITE_THROUGH : 1;      // bit 29
        ULONG64 CACHE_DISABLE : 1;          // bit 30
        ULONG64 PAGING : 1;                 // bit 31
        ULONG64 __reserved4 : 32;           // bits 32 : 63
    };
} CR0_T, * PCR0_T;

ULONG64
BLAPI
ReadCR0(

)
{
    return __readcr0( );
}

VOID
BLAPI
WriteCR0(
    ULONG64 Value
)
{
    __writecr0( Value );
}

//
// CR2
//

//
// CR3
//

#define CR3_PAGE_LEVEL_WRITE_THROUGH (1ull << 3)
#define CR3_PAGE_LEVEL_CACHE_DISABLE (1ull << 4)

//typedef enum _CR3_FLAGS : ULONG64
//{
//    // enable to use write-through for page table, other wise use write-back
//    PAGE_LEVEL_WRITE_THROUGH = 1 << 3,
//
//    // disable caching for the page table
//    PAGE_LEVEL_CACHE_DISABLE = 1 << 4
//} CR3_FLAGS;

//
// CR4
//

#define CR4_PAGE_SIZE_EXENTSION (1ull << 4)
#define CR4_PHYSICAL_ADDRESS_EXTENSION (1ull << 5)
#define CR4_PCID (1ull << 17)

//typedef enum _CR4_FLAGS : ULONG64
//{
//    PAGE_SIZE_EXTENSION = 1 << 4,
//
//    // (PAE)
//    PHYSICAL_ADDRESS_EXTENSION = 1 << 5,
//
//
//    PCID = 1 << 17
//};
