#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include "boot.h"
#include "pe.h"
#include <Library/BaseMemoryLib.h>

typedef union _VIRT_ADDR_T
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
} VIRT_ADDR_T, * PVIRT_ADDR_T;

typedef union _plm4e
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;          // bits [ 0 ]
        ULONG64 Rw : 1;               // bits [ 1 ]
        ULONG64 UserAccess : 1;       // bits [ 2 ]
        ULONG64 PageWriteThrough : 1; // bits [ 3 ]
        ULONG64 PageCache : 1;        // bits [ 4 ]
        ULONG64 Accessed : 1;         // bits [ 5 ]
        ULONG64 __ignored1 : 1;       // bits [ 6 ]
        ULONG64 PageSize : 1;         // bits [ 7 ] /* (must be 0 for plm4) */
        ULONG64 __ignored2 : 1;       // bits [ 8 ]
        ULONG64 Pfn : 36;             // bits [ 9:44 ]
        ULONG64 __reserved : 4;       // bits [ 45:48 ]
        ULONG64 __ignored3 : 11;      // bits [ 49:59 ]
        ULONG64 Nx : 1;               // bits [ 60 ]
    };
} plm4e, * pplm4e;

typedef plm4e PML4E;
typedef pplm4e PPML4E;

typedef union _pdpte
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;     // bits [ 0 ]
        ULONG64 Rw : 1;          // bits [ 1 ]
        ULONG64 UserAccess : 1;  // bits [ 2 ]
        ULONG64 PageWrite : 1;   // bits [ 3 ]
        ULONG64 PageCache : 1;   // bits [ 4 ]
        ULONG64 Accessed : 1;    // bits [ 5 ]
        ULONG64 __ignored1 : 1;  // bits [ 6 ]
        ULONG64 PageSize : 1;    // bits [ 7 ] /* (if set maps 1GB, otherwise points to PD) */   
        ULONG64 __ignored2 : 1;  // bits [ 8 ]
        ULONG64 Pfn : 36;        // bits [ 9:44 ]
        ULONG64 __reserved : 4;  // bits [ 45:48 ]
        ULONG64 __ignored3 : 11; // bits [ 49:59 ]
        ULONG64 Nx : 1;          // bits [ 60 ]
    };
} pdpte, * ppdpte;

typedef pdpte PDPTE;
typedef ppdpte PPDPTE;

typedef union _pde
{
    ULONG64 Value;
    struct
    {
        ULONG64 Present : 1;      // bits [ 0 ]
        ULONG64 Rw : 1;           // bits [ 1 ]
        ULONG64 UserAccess : 1;   // bits [ 2 ]
        ULONG64 PageWrite : 1;    // bits [ 3 ]
        ULONG64 PageCache : 1;    // bits [ 4 ]
        ULONG64 Accessed : 1;     // bits [ 5 ]
        ULONG64 __ignored1 : 1;   // bits [ 6 ]
        ULONG64 PageSize : 1;     // bits [ 7 ] /* (if set maps 2MB otherwise points to PT) */
        ULONG64 __ignored2 : 1;   // bits [ 8 ]
        ULONG64 Pfn : 36;         // bits [ 9:44 ]
        ULONG64 __reserved : 4;   // bits [ 45:48 ]
        ULONG64 __ignored3 : 11;  // bits [ 49:59 ]
        ULONG64 Nx : 1;           // bits [ 60 ]
    };
} pde, * ppde;

typedef pde PDE;
typedef ppde PPDE;

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

typedef enum _PFN_STATE
{
    Free,
    Allocated,
    Reserved
} PFN_STATE;

typedef struct _PFN_ENTRY
{
    PFN_STATE State;
    UINT32 Offset;
    UINT32 Ref;
} PFN_ENTRY, *PPFN_ENTRY;

PPFN_ENTRY SsPfn;
ULONG64    SsPfnCount;
ULONG64    SsPfnFreeHead;


#define DEFAULT_PAGE_SIZE 0x1000
#define LARGE_PAGE_SIZE   0x200000
#define LARGEST_PAGE_SIZE 0x80000000

#define PAGE_SHIFT 12

#define PFN_TO_PHYSICAL_SIZE( pfn, size ) ((pfn) << size) 
#define PFN_TO_PHYSICAL( pfn ) ( PFN_TO_PHYSICAL_SIZE( pfn, PAGE_SHIFT ) )

#define PHYSICAL_TO_PFN_SIZE( adr, size ) ((adr) >> size)
#define PHYSICAL_TO_PFN( adr ) ( PHYSICAL_TO_PFN_SIZE( adr, PAGE_SHIFT ) )

#define LOW_MEMORY_START  0x0000000000000000ULL
#define LOW_MEMORY_END    0x00007FFFFFFFFFFFULL

#define HIGH_MEMORY_START 0xFFFF800000000000ULL
#define HIGH_MEMORY_END   0xFFFFFFFFFFFFFFFFULL

#define DIRECT_MAP_BASE HIGH_MEMORY_START

#define KERNEL_VA_BASE HIGH_MEMORY_START + (LARGEST_PAGE_SIZE * 100)

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


/**
* Converst a physical address to a virtual address that
*/
STATIC 
__forceinline
VOID*
BLAPI
PhysicalToVirtual(
    _In_ ULONG64 PhysicalAddress
)
{
    DEBUG_ASSERT(PhysicalAddress, NULL, "Expected valid paramter");
    return (VOID*)(DIRECT_MAP_BASE + PhysicalAddress);
}

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

        sizeof(UINT32);
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

        Initalised = TRUE;
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


ULONG64
BLAPI
SsGetFreePhysicalPage(
    VOID
)
{
    if (SsPfnFreeHead == 0xffffff)
    {
        DEBUG_INFO(L"No Free memory!");
        getc();
        return 0;
    }

    ULONG64 PageBase = PFN_TO_PHYSICAL(SsPfnFreeHead);
    PFN_ENTRY* Entry = &SsPfn[SsPfnFreeHead];

    // set next head
    SsPfnFreeHead = Entry->Offset;

    Entry->State = Allocated;
    Entry->Ref = 1;
    Entry->Offset = 0xffffff;

    return PageBase;
}

static ULONG64* gPML4;

VOID
BLAPI
SsFreePhysicalPage(
    _In_ ULONG64 Address
)
{
    ULONG64 IndexPFN = PHYSICAL_TO_PFN( Address );

    if(IndexPFN >= SsPfnCount );
    {
        DEBUG_INFO( L"Pfn too large\n" );
        getc();
        return;
    }

    PFN_ENTRY* PFN = &SsPfn[ IndexPFN ];
    PFN->State = Free;
    PFN->Ref = 0;
    PFN->Offset = (UINT32)SsPfnFreeHead;
    SsPfnFreeHead = (ULONG64)PFN;
};


// DirectPagingInit initializes our paging structures for the direct map.
// It allocates one page for the PML4 (level-4 table) and zeros it.
VOID
BLAPI
SsPagingInit(
    VOID
)
{
    ULONG64 FreePage = SsGetFreePhysicalPage();

    if (!FreePage)
    {
        DEBUG_INFO(L"No free memory!");
        getc();
        return;
    }

    gPML4 =  (ULONG64*)PhysicalToVirtual(FreePage);

    ZeroMem(gPML4, DEFAULT_PAGE_SIZE);
}

// MapPage maps a single 4KB page so that virtual address 'vaddr'
// maps to physical address 'paddr' with the specified 'flags' (for the PTE).
// This function walks the 4-level page table hierarchy, allocating lower-level tables on demand.
// It uses our global gPML4 (which is assumed to be already initialized).
EFI_STATUS MapPage(UINT64 vaddr, UINT64 paddr, UINT64 flags)
{
    // Decompose vaddr into indices:
    UINT64 pml4_index = (vaddr >> 39) & 0x1FF;
    UINT64 pdpt_index = (vaddr >> 30) & 0x1FF;
    UINT64 pd_index = (vaddr >> 21) & 0x1FF;
    UINT64 pt_index = (vaddr >> 12) & 0x1FF;

    // Get or allocate the PDPT.
    PPML4E pml4e = ((PPML4E)&gPML4[pml4_index]);
    UINT64 pdptPhys;
    PPDPTE pdptTable;
    if (!(pml4e->Present)) {
        EFI_STATUS Status = AllocatePage(&pdptPhys);
        if (EFI_ERROR(Status))
            return Status;
        ZeroMem(PhysicalToVirtual(pdptPhys), DEFAULT_PAGE_SIZE);
        pml4e->Value = (pdptPhys & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;
    }
    pdptTable = (PPDPTE)PhysicalToVirtual(pml4e->Pfn << PAGE_SHIFT);

    // Get or allocate the PD.
    PPDPTE pdpe = &pdptTable[pdpt_index];
    UINT64 pdPhys;
    PPDE pdTable;
    if (!(pdpe->Present)) {
        EFI_STATUS Status = AllocatePage(&pdPhys);
        if (EFI_ERROR(Status))
            return Status;
        ZeroMem(PhysicalToVirtual(pdPhys), DEFAULT_PAGE_SIZE);
        pdpe->Value = (pdPhys & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;
    }
    pdTable = (PPDE)PhysicalToVirtual(pdpe->Pfn << PAGE_SHIFT);

    // Get or allocate the PT.
    PPDE pde = &pdTable[pd_index];
    UINT64 ptPhys;
    PPTE ptTable;
    if (!(pde->Present)) {
        EFI_STATUS Status = AllocatePage(&ptPhys);
        if (EFI_ERROR(Status))
            return Status;
        ZeroMem(PhysicalToVirtual(ptPhys), DEFAULT_PAGE_SIZE);
        pde->Value = (ptPhys & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;
    }
    ptTable = (PPTE)PhysicalToVirtual(pde->Pfn << PAGE_SHIFT);

    // Finally, install the PTE for the requested mapping.
    PPTE pte = &ptTable[pt_index];
    pte->Value = (paddr & 0xFFFFFFFFFFFFF000ULL) | flags;
    return EFI_SUCCESS;
}

// DirectMapRange maps all physical memory from physStart to physEnd
// into the direct mapped region so that VA = DIRECT_MAP_BASE + PA.
// The caller must ensure that physStart and physEnd are page-aligned.
EFI_STATUS DirectMapRange(UINT64 physStart, UINT64 physEnd)
{
    if ((physStart % DEFAULT_PAGE_SIZE) || (physEnd % DEFAULT_PAGE_SIZE))
        return EFI_INVALID_PARAMETER;

    for (UINT64 phys = physStart; phys < physEnd; phys += DEFAULT_PAGE_SIZE)
    {
        UINT64 vaddr = DIRECT_MAP_BASE + phys;
        EFI_STATUS Status = MapPage(vaddr, phys, FLAG_PRESENT | FLAG_RW | FLAG_NX);
        if (EFI_ERROR(Status))
        {
            return Status;
        }
    }
    ReloadCR3(); // Flush the TLB
    return EFI_SUCCESS;
}

#endif
