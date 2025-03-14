#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include "boot.h";
#include "pe.h"
#include <Library/BaseMemoryLib.h>

typedef union _virt_addr_t
{
    PVOID Value;
    struct
    {
        ULONG64 Offset : 12;
        ULONG64 PtIndex : 9;
        ULONG64 PdIndex : 9;
        ULONG64 PdptIndex : 9;
        ULONG64 Plm4Index : 9;
        ULONG64 __reserved : 16;
    };
} virt_addr_t, * pvirt_addr_t;

typedef virt_addr_t VIRT_ADDR_T;
typedef pvirt_addr_t PVIRT_ADDR_T;

typedef union _plm4e
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;
        ULONG64 Rw : 1;
        ULONG64 UserAccess : 1;
        ULONG64 PageWriteThrough : 1;
        ULONG64 PageCache : 1;
        ULONG64 Accessed : 1;
        ULONG64 __ignored1 : 1;
        ULONG64 PageSize : 1;
        ULONG64 __ignored2 : 1;
        ULONG64 Pfn : 36;
        ULONG64 __reserved : 4;
        ULONG64 __ignored3 : 11;
        ULONG64 Nx : 1;
    };
} plm4e, * pplm4e;

typedef plm4e PLM4E;
typedef pplm4e PPLM4E;

typedef union _pdpte
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;
        ULONG64 Rw : 1;
        ULONG64 UserAccess : 1;
        ULONG64 PageWrite : 1;
        ULONG64 PageCache : 1;
        ULONG64 Accessed : 1;
        ULONG64 __ignored1 : 1;
        ULONG64 PageSize : 1;
        ULONG64 __ignored2 : 1;
        ULONG64 Pfn : 36;
        ULONG64 __reserved : 4;
        ULONG64 __ignored3 : 11;
        ULONG64 Nx : 1;
    };
} pdpte, * ppdpte;

typedef pdpte PDPTE;
typedef ppdpte PPDPTE;

typedef union _pde
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;
        ULONG64 Rw : 1;
        ULONG64 UserAccess : 1;
        ULONG64 PageWrite : 1;
        ULONG64 PageCache : 1;
        ULONG64 Accessed : 1;
        ULONG64 __ignored1 : 1;
        ULONG64 PageSize : 1;
        ULONG64 __ignored2 : 1;
        ULONG64 Pfn : 36;
        ULONG64 __reserved : 4;
        ULONG64 __ignored3 : 11;
        ULONG64 Nx : 1;
    };
} pde, * ppde;

typedef union pde PDE;
typedef union ppde PPDE;

typedef union _pte
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;
        ULONG64 Rw : 1;
        ULONG64 UserAccess : 1;
        ULONG64 PageWrite : 1;
        ULONG64 PageCache : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 PageAccessType : 1;
        ULONG64 Global : 1;
        ULONG64 __ignored1 : 3;
        ULONG64 Pfn : 36;
        ULONG64 __reserved : 4;
        ULONG64 __ignored3 : 7;
        ULONG64 ProtectedKey : 4;
        ULONG64 Nx : 1;
    };
} pte, * ppte;

typedef pte PTE;
typedef ppte PPTE;

STATIC ULONG64 __pml4, __pdpt, __pd, __pt;

STATIC UINT64* pml4 = NULL;
STATIC UINT64* pdpt = NULL;
STATIC UINT64* pd = NULL;
STATIC UINT64* pt = NULL;

// flag bits for entries
const ULONG64 FLAG_PRESENT = 1ULL << 0;
const ULONG64 FLAG_RW = 1ULL << 1;
const ULONG64 FLAG_USER = 1ULL << 2;  // keep this 0 for kernel
const ULONG64 FLAG_PS = 1ULL << 7;    // page size (for PD/PT entries)
const ULONG64 FLAG_NX = 1ULL << 63;   // No-execute (bit 63)

#define PAGESIZE 4096

typedef struct _BL_EFI_MEMORY_MAP
{
    EFI_MEMORY_DESCRIPTOR* Descriptor;
    ULONG64 MapSize;
    ULONG64 Key;
    ULONG64 DescriptorSize;
    UINT32  Version;
} BL_EFI_MEMORY_MAP, * PBL_EFI_MEMORY_MAP;

inline STATIC
VOID
BLAPI
BlIntialisePageManager(
    VOID
)
{
    STATIC BOOLEAN Initalised = FALSE;

    if (!Initalised)
    {
        //
        // allocate a page for each paging structure
        //
        gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &__pml4);
        gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &__pdpt);
        gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &__pd);
        gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &__pt);

        pml4 = (ULONG64*)(UINTN)__pml4;
        pdpt = (ULONG64*)(UINTN)__pdpt;
        pd = (ULONG64*)(UINTN)__pd;
        pt = (ULONG64*)(UINTN)__pt;

        //
        // Zero out all pages
        //
        ZeroMem(pml4, 4096);
        ZeroMem(pdpt, 4096);
        ZeroMem(pd, 4096);
        ZeroMem(pt, 4096);

        pml4[511] = (__pdpt & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);         // PML4 entry for high-half
        pdpt[510] = (__pd & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);
        pd[0] = (__pt & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);

        Initalised = true;
    }
}

inline STATIC
VOID
BLAPI
BlMapKernelPages(
    _In_ ULONG64 Image
)
{
    if (PeIsValidImage((PBYTE)Image))
    {
        return;
    }

    EFI_IMAGE_NT_HEADERS* ImageNtHeaders = EFI_IMAGE_NTHEADERS(Image);

    for (ULONG i = 0; i < ImageNtHeaders->OptionalHeader.SizeOfImage / 4096; ++i)
    {
        ULONG64 PhysicalPage = Image + i * 4096ull;
        pt[i] = (PhysicalPage & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);
    }
}


#endif
