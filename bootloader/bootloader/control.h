#pragma once

#include "registerutil.h"

//
// CR0 
//

typedef enum _CR0_FLAGS : ULONG64
{
    // enables protected mode
    PROTECTED_MODE_ENABLE = 1ull,

    // enables monitoring of coprocessor
    MONITOR_COPROCESSOR = 1ull << 1,

    //force all x87 and MMX instructions to cause a 'Device Not Found' exception
    EMULATE_COPROCESSOR = 1ull << 2,

    //allows saving x87/MMX/SSE instructions on hardware context switches
    TASK_SWITCHED = 1ull << 3,

    //checks for support of 387DX math coprocessor instruction
    EXTENSION_TYPE = 1ull << 4,

    //enables internel error reporting mechanism for x87 FPU errors
    NUMERIC_ERROR = 1ull << 5,

    // when set, it is not possible to write read-only pages
    WRITE_PROTECT = 1ull << 16,

    //allows for checking alignment in usermode 
    ALIGNMENT_MASK = 1ull << 18,

    //older cpus use this to control write-back/write-through cache strategy
    NOT_WRITE_THROUGH = 1ull << 29,

    //disable some processor caches, specifics model-dependent
    CACHE_DISABLE = 1ull << 30,

    //enables paging, PROTECTED MODE MUST BE ENABLED!
    PAGING = 1ull << 31
} CR0_FLAGS;

typedef struct _REG_CR0 
{
    union 
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
            ULONG64 reserved1 : 9;              // bits 6 : 15
            ULONG64 WRITE_PROTECT : 1;          // bit 16
            ULONG64 reserved2 : 1;              // bit 17
            ULONG64 ALIGNMENT_MASK : 1;         // bit 18
            ULONG64 reserved3 : 10;             // bits 19 : 28
            ULONG64 NOT_WRITE_THROUGH : 1;      // bit 29
            ULONG64 CACHE_DISABLE : 1;          // bit 30
            ULONG64 PAGING : 1;                 // bit 31
            ULONG64 reserved4 : 32;             // bits 32 : 63
        };
    };
} REG_CR0, * PREG_CR0;

REG_CR0
BLAPI
ReadCR0(

)
{
    REG_CR0 cr0 = { __readcr0() };
    return cr0;
}

VOID
BLAPI
WriteCR0(
    REG_CR0 cr0
)
{
    __writecr0(cr0.Value);
}

VOID
BLAPI 
SetCR0Flag(
    _In_ CR0_FLAGS FlagsSet
) 
{
    REG_CR0 cr0 = ReadCR0();
    // set specified flags
    cr0.Value |= (ULONG64)FlagsSet;
    WriteCR0(cr0);
}

VOID 
BLAPI
ClearCR0Flag(
    _In_ CR0_FLAGS FlagsClear
)
{
    REG_CR0 cr0 = ReadCR0();
    cr0.Value &= ~((ULONG64)FlagsClear);
    WriteCR0(cr0);
}

//
// CR2
//

//
// CR3
//

typedef enum _CR3_FLAGS : ULONG64
{
    // enable to use write-through for page table, other wise use write-back
    PAGE_LEVEL_WRITE_THROUGH = 1 << 3,

    // disable caching for the page table
    PAGE_LEVEL_CACHE_DISABLE = 1 << 4
} CR3_FLAGS;

//
// CR4
//

typedef enum _CR4_FLAGS : ULONG64
{
    PAGE_SIZE_EXTENSION = 1 << 4,

    // (PAE)
    PHYSICAL_ADDRESS_EXTENSION = 1 << 5,


    PCID = 1 << 17
};
