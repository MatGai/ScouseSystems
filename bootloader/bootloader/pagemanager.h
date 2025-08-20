#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include "boot.h"
#include "pe.h"
#include <Library/BaseMemoryLib.h>
#include "general.h"

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
#define LARGEST_PAGE_SIZE 0x40000000

#define PAGE_SHIFT 12
#define PFN_LIST_END 0xffffff

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

typedef struct _BL_EFI_MEMORY_MAP
{
    EFI_MEMORY_DESCRIPTOR* Descriptor;
    ULONG64 MapSize;
    ULONG64 Key;
    ULONG64 DescriptorSize;
    UINT32  Version;
} BL_EFI_MEMORY_MAP, * PBL_EFI_MEMORY_MAP;

/**
* Converst a physical address to a virtual address 
* in the direct mapped region.
* Only works after paging has been initialized.
* 
* @param PhysicalAddress A physical address to convert.
* 
* @return A virtual address in the direct mapped region.
*/
STATIC 
__forceinline
VOID*
BLAPI
PhysicalToVirtual(
    _In_ ULONG64 PhysicalAddress
)
{
    DBG_ASSERT(PhysicalAddress, NULL, "Expected valid paramter");
    return (VOID*)(DIRECT_MAP_BASE + PhysicalAddress);
}


ULONG64
BLAPI
SsGetFreePhysicalPage(
    VOID
)
{
    if (SsPfnFreeHead == PFN_LIST_END)
    {
        DBG_INFO(L"No Free memory!");
        getc();
        return 0;
    }

    ULONG64 PageBase = PFN_TO_PHYSICAL(SsPfnFreeHead);
    PFN_ENTRY* Entry = &SsPfn[SsPfnFreeHead];

    // set next head
    SsPfnFreeHead = Entry->Offset;

    Entry->State = Allocated;
    Entry->Ref = 1;
    Entry->Offset = PFN_LIST_END;

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

    if(IndexPFN >= SsPfnCount )
    {
        DBG_INFO( L"Pfn too large\n" );
        getc();
        return;
    }

    PFN_ENTRY* PFN = &SsPfn[ IndexPFN ];
    PFN->State = Free;
    PFN->Ref = 0;
    PFN->Offset = (UINT32)SsPfnFreeHead;
    SsPfnFreeHead = IndexPFN;
};


// DirectPagingInit initializes our paging structures for the direct map.
// It allocates one page for the PML4 (level-4 table) and zeros it.
ULONG64
BLAPI
SsPagingInit(
    VOID
)
{
    ULONG64 FreePage = SsGetFreePhysicalPage();

    if (!FreePage)
    {
        DBG_INFO(L"No free memory!");
        getc();
        return NULL;
    }

    gPML4 =  (ULONG64*)(PVOID)FreePage;

    ZeroMem(gPML4, DEFAULT_PAGE_SIZE);

    return FreePage;
}

// MapPage maps a single 4KB page so that virtual address 'vaddr'
// maps to physical address 'paddr' with the specified 'flags' (for the PTE).
// This function walks the 4-level page table hierarchy, allocating lower-level tables on demand.
// It uses our global gPML4 (which is assumed to be already initialized).
EFI_STATUS 
MapPage(
    ULONG64 vaddr, 
    ULONG64 paddr, 
    ULONG64 flags
)
{
    // Decompose vaddr into indices:
    //UINT64 pml4_index = (vaddr >> 39) & 0x1FF;
    //UINT64 pdpt_index = (vaddr >> 30) & 0x1FF;
    //UINT64 pd_index = (vaddr >> 21) & 0x1FF;
    //UINT64 pt_index = (vaddr >> 12) & 0x1FF;

    PVIRT_ADDR_T VirtualAddress = { (PVOID)vaddr };

    PPML4E Pml4e = ((PPML4E)&gPML4[VirtualAddress->Plm4Index]);
    PPDPTE PdptTable;
    if (!(Pml4e->Present)) 
    {
        ULONG64 PdptPhysical = SsGetFreePhysicalPage();
        if (!PdptPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        PVOID PdptVirtual = PhysicalToVirtual(PdptPhysical);
        ZeroMem((PVOID)PdptVirtual, DEFAULT_PAGE_SIZE);

        Pml4e->Value = (PdptPhysical & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;

        mfence();
    }
    PdptTable = (PPDPTE)PhysicalToVirtual(Pml4e->Pfn << PAGE_SHIFT);

    PPDPTE Pdpe = &PdptTable[VirtualAddress->PdptIndex];
    PPDE PdTable;
    if (!(Pdpe->Present)) 
    {
        ULONG64 PdPhysical = SsGetFreePhysicalPage();
        if (!PdPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        PVOID PdVirtual = PhysicalToVirtual(PdPhysical);
        ZeroMem(PdVirtual, DEFAULT_PAGE_SIZE);

        Pdpe->Value = (PdPhysical & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;

        mfence();
    }
    PdTable = (PPDE)PhysicalToVirtual(Pdpe->Pfn << PAGE_SHIFT);

    PPDE Pde = &PdTable[VirtualAddress->PdIndex];
    PPTE PtTable;
    if (!(Pde->Present)) 
    {
        ULONG64 PtPhysical = SsGetFreePhysicalPage();
        if (!PtPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        PVOID PtVirtual = PhysicalToVirtual(PtPhysical);
        ZeroMem(PtVirtual, DEFAULT_PAGE_SIZE);

        Pde->Value = (PtPhysical & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;

        mfence();
    }
    PtTable = (PPTE)PhysicalToVirtual(Pde->Pfn << PAGE_SHIFT);

    PPTE Pte = &PtTable[VirtualAddress->PtIndex];
    Pte->Value = (paddr & 0xFFFFFFFFFFFFF000ULL) | flags;
    tlbflush((VOID*)vaddr);
    mfence();
    return EFI_SUCCESS;
}

// MapLargePage maps a single 2MB or 1GB page, depending on 'size'.
// Size must be either LARGE_PAGE_SIZE or LARGEST_PAGE_SIZE
EFI_STATUS
MapLargePage(
    ULONG64 vaddr,
    ULONG64 paddr,
    ULONG64 size,
    ULONG64 flags
)
{
    VIRT_ADDR_T VirtualAddress = { .Value = (PVOID)vaddr };

    Print(L"\nHelp 0 vaddr %p, va %p\n", vaddr, VirtualAddress);
    getc();

    Print(L"Plm4e %p\n", VirtualAddress.Plm4Index);
    getc();

    PPML4E Pml4e = &gPML4[VirtualAddress.Plm4Index];
    if (!(Pml4e->Present))
    {

        Print(L"Help 0.1\n");
        getc();

        ULONG64 PdptPhysical = SsGetFreePhysicalPage();
        if (!PdptPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        Print(L"Help 0.2 %p\n", PdptPhysical);
        getc();

        PVOID PdptVirtual = PhysicalToVirtual(PdptPhysical);
        ZeroMem(PdptVirtual, DEFAULT_PAGE_SIZE);

        Print(L"Help 0.3\n");
        getc();

        Pml4e->Value = (PdptPhysical & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;

        Print(L"Help 0.4\n");
        getc();

        tlbflush(PdptVirtual);
        mfence();

        Print(L"Help 0.5\n");
        getc();
    }

    PPDPTE PdptTable = (PPDPTE)PhysicalToVirtual(Pml4e->Pfn << PAGE_SHIFT);

    Print(L"Help 1\n");
    getc();

    if (size == LARGEST_PAGE_SIZE)
    {
        PPDPTE Pdpe = &PdptTable[VirtualAddress.PdptIndex];
        Pdpe->Value = (paddr & ~(LARGEST_PAGE_SIZE - 1ULL)) | flags | FLAG_PS;
        return EFI_SUCCESS;
    }

    PPDPTE Pdpe = &PdptTable[VirtualAddress.PdptIndex];
    if (!(Pdpe->Present))
    {
        ULONG64 PdPhysical = SsGetFreePhysicalPage();
        if (!PdPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        PVOID PdVirtual = PhysicalToVirtual(PdPhysical);
        ZeroMem(PdVirtual, DEFAULT_PAGE_SIZE);

        Pdpe->Value = (PdPhysical & 0xFFFFFFFFFFFFF000ULL) | FLAG_PRESENT | FLAG_RW;

        tlbflush(PdVirtual);
        mfence();
    }

    Print(L"Help 2\n");
    getc();

    if (size == LARGE_PAGE_SIZE)
    {
        PPDE PdTable = (PPDE)PhysicalToVirtual(Pdpe->Pfn << PAGE_SHIFT);
        PPDE Pde = &PdTable[VirtualAddress.PdIndex];
        Pde->Value = (paddr & ~(LARGE_PAGE_SIZE - 1ULL)) | flags | FLAG_PS;
        return EFI_SUCCESS;
    }

    return MapPage(vaddr, paddr, flags);
}


// DirectMapRange maps all physical memory from physStart to physEnd
// into the direct mapped region so that VA = DIRECT_MAP_BASE + PA.
// The caller must ensure that physStart and physEnd are page-aligned.
EFI_STATUS DirectMapRange(
    ULONG64 PhysStart, 
    ULONG64 PhysEnd
)
{
    if ((PhysStart % DEFAULT_PAGE_SIZE) || (PhysEnd % DEFAULT_PAGE_SIZE))
    {
        return EFI_INVALID_PARAMETER;
    }

    Print(L"\nPhys start %p, end %p\n", PhysStart, PhysEnd);
    getc();

    for (UINT64 Phys = PhysStart; Phys < PhysEnd;)
    {
        ULONG64 Remaining = PhysEnd - Phys;
        ULONG64 vaddr = DIRECT_MAP_BASE + Phys;
        EFI_STATUS Status;

        Print(L"Fail 0");
        getc();

        if ((Phys % LARGEST_PAGE_SIZE) == 0 && Remaining >= LARGEST_PAGE_SIZE)
        {
            Print(L"Fail 1");
            getc();

            Print(L"vaddr %p, phys %p, remaining %p", vaddr, Phys, Remaining);
            Status = MapLargePage(vaddr, Phys, LARGEST_PAGE_SIZE, FLAG_PRESENT | FLAG_RW | FLAG_NX);

            Print(L"Fail 1.1");
            getc();

            if (EFI_ERROR(Status))
            {
                Print(L"Fail 1.2 error");
                getc();
                return Status;
            }

            Print(L"Fail 1.3 ");
            getc();

            Phys += LARGEST_PAGE_SIZE;
        }
        else if ((Phys % LARGE_PAGE_SIZE) == 0 && Remaining >= LARGE_PAGE_SIZE)
        {

            Print(L"Fail 2");
            getc();

            Status = MapLargePage(vaddr, Phys, LARGE_PAGE_SIZE, FLAG_PRESENT | FLAG_RW | FLAG_NX);
            if (EFI_ERROR(Status))
            {
                return Status;
            }
            Phys += LARGE_PAGE_SIZE;
        }
        else
        {

            Print(L"Fail 3");
            getc();

            Status = MapPage(vaddr, Phys, FLAG_PRESENT | FLAG_RW | FLAG_NX);
            if (EFI_ERROR(Status))
            {
                return Status;
            }
            Phys += DEFAULT_PAGE_SIZE;
        }

        Print(L"Fail 4");
        getc();
    }
    //ReloadCR3(); // Flush the TLB
    return EFI_SUCCESS;
}

#endif
