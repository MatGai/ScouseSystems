#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include "boot.h";
#include "pe.h"
#include <Library/BaseMemoryLib.h>

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
} BL_EFI_MEMORY_MAP, * BL_EFI_MEMORY_MAP;

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

        pml4  = (ULONG64*)(UINTN)__pml4;
        pdpt = (ULONG64*)(UINTN)__pdpt;
        pd   = (ULONG64*)(UINTN)__pd;
        pt   = (ULONG64*)(UINTN)__pt;

        //
        // Zero out all pages
        //
        ZeroMem(pml4, 4096);
        ZeroMem(pdpt, 4096);
        ZeroMem(pd, 4096);
        ZeroMem(pt, 4096);

        pml4[511]  = (__pdpt & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);         // PML4 entry for high-half
        pdpt[510] = (__pd & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);
        pd[0]     = (__pt & 0xFFFFFFFFFFFFF000ULL) | (FLAG_PRESENT | FLAG_RW);

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
