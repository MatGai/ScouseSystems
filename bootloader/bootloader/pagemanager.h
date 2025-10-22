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

#define PLM4_INDEX( va )  ( ( (ULONG64)(va) >> 39 ) & 0x1FF )
#define PDPT_INDEX( va )  ( ( (ULONG64)(va) >> 30 ) & 0x1FF )
#define PD_INDEX( va )    ( ( (ULONG64)(va) >> 21 ) & 0x1FF )
#define PT_INDEX( va )    ( ( (ULONG64)(va) >> 12 ) & 0x1FF )

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

#define KERNEL_VA_BASE 0xFFFFFFFF80000000ULL

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

STATIC
__forceinline
ULONG64
BLAPI
EntryAddress(
    _In_ ULONG64 Entry
)
{
    return Entry & 0x000FFFFFFFFFF000ULL;
}

STATIC 
__forceinline
ULONG64
BLAPI
MakeEntry(
    _In_ ULONG64 Address,
    _In_ ULONG64 Flags
)
{
    return (Address & 0x000FFFFFFFFFF000ULL) | (Flags & ~0xFFFULL);
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
// had issues with this, I was trying to get the virtual address of physical addresses, when paging was still not setup. 
// We are still in direct mapping where phsyical address = virtual!
{
    // Decompose vaddr to get table indexs
    ULONG64 pml4_index = PLM4_INDEX( vaddr );
    ULONG64 pdpt_index = PDPT_INDEX( vaddr );
    ULONG64 pd_index   = PD_INDEX( vaddr );
    ULONG64 pt_index   = PT_INDEX( vaddr );


    Print(L"vaddr %p, paddr %p, size %p\n", vaddr, paddr, 0x1000);
    Print(L"plm4indx %p, pdptindx %p, pdindx %p, ptindx %p\n", pml4_index, pdpt_index, pd_index, pt_index);


    PVIRT_ADDR_T VirtualAddress = { (PVOID)vaddr };

    ULONG64* Pml4e = (ULONG64*)(ULONG64)gPML4;
    ULONG64* PdptTable;
    if (!(Pml4e[pml4_index] & FLAG_PRESENT))
    {
        ULONG64 PdptPhysical = SsGetFreePhysicalPage();
        if (!PdptPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        //PVOID PdptVirtual = PhysicalToVirtual(PdptPhysical);
        ZeroMem((PVOID)(ULONG64)PdptPhysical, DEFAULT_PAGE_SIZE);

        Pml4e[pml4_index] = MakeEntry(PdptPhysical, FLAG_PRESENT | FLAG_RW);

        mfence();
    }
    PdptTable = (ULONG64*)(ULONG64)EntryAddress(Pml4e[pml4_index]);

    ULONG64* Pdpe = &PdptTable[pdpt_index];
    ULONG64* PdTable;
    if (!(*Pdpe & FLAG_PRESENT)) 
    {
        ULONG64 PdPhysical = SsGetFreePhysicalPage();
        if (!PdPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        //PVOID PdVirtual = PhysicalToVirtual(PdPhysical);
        ZeroMem((PVOID)(ULONG64)PdPhysical, DEFAULT_PAGE_SIZE);
        *Pdpe = MakeEntry(PdPhysical, FLAG_PRESENT | FLAG_RW );
        mfence();
    }
    PdTable = (ULONG64*)(ULONG64)EntryAddress(*Pdpe);/*(PPDE)PhysicalToVirtual(Pdpe->Pfn << PAGE_SHIFT);*/

    ULONG64* Pde = &PdTable[pd_index];
    ULONG64* PtTable;
    if (!(*Pde & FLAG_PRESENT)) 
    {
        ULONG64 PtPhysical = SsGetFreePhysicalPage();
        if (!PtPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        /*PVOID PtVirtual = PhysicalToVirtual(PtPhysical);*/
        ZeroMem((PVOID)(ULONG64)PtPhysical, DEFAULT_PAGE_SIZE);
        *Pde = MakeEntry(PtPhysical, FLAG_PRESENT | FLAG_RW);
        mfence();
    }
    PtTable = (ULONG64*)(ULONG64)EntryAddress(*Pde);

    // Make PS is not set at PT level, only PD/PT entries use PS bit for large pages
    PtTable[pt_index] = MakeEntry(paddr, flags & ~FLAG_PS);

    // not really needed as we are not modifying cr3 yet.
    //tlbflush((VOID*)(ULONG64)vaddr);
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
    ULONG64 pml4_index = PLM4_INDEX(vaddr);
    ULONG64 pdpt_index = PDPT_INDEX(vaddr);
    ULONG64 pd_index   = PD_INDEX(vaddr);
    ULONG64 pt_index   = PT_INDEX(vaddr);

    Print(L"vaddr %p, paddr %p, size %p\n", vaddr, paddr, size);
    Print(L"plm4indx %p, pdptindx %p, pdindx %p, ptindx %p\n", pml4_index, pdpt_index, pd_index, pt_index);

    getc();

    ULONG64* Pml4 = (ULONG64*)(ULONG64)gPML4;
    ULONG64* PdpTable;
    if (!(Pml4[pml4_index] & FLAG_PRESENT))
    {
        ULONG64 PdptPhysical = SsGetFreePhysicalPage();
        if (!PdptPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        //PVOID PdptVirtual = PhysicalToVirtual(PdptPhysical);
        ZeroMem((PVOID)(ULONG64)PdptPhysical, DEFAULT_PAGE_SIZE);


        Pml4[pml4_index] = MakeEntry(PdptPhysical, FLAG_PRESENT | FLAG_RW);

        mfence();
    }

    PdpTable = (ULONG64*)(ULONG64)EntryAddress(Pml4[pml4_index]);


    if (size == LARGEST_PAGE_SIZE)
    {
        if ((paddr & (LARGEST_PAGE_SIZE - 1ULL)) != 0 || (vaddr & (LARGEST_PAGE_SIZE - 1)) != 0)
        {
            DBG_INFO(L"Physical address not aligned for 1GB page");
            getc();
            return EFI_INVALID_PARAMETER;
        }

        PdpTable[pdpt_index] = MakeEntry(paddr, (flags | FLAG_PS));
        mfence();
        return EFI_SUCCESS;
    }

    // PDPT -> PD
    ULONG64* Pdpe = &PdpTable[pdpt_index];
    if (!(*Pdpe & FLAG_PRESENT))
    {
        ULONG64 PdPhysical = SsGetFreePhysicalPage();
        if (!PdPhysical)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        //PVOID PdVirtual = PhysicalToVirtual(PdPhysical);
        ZeroMem((PVOID)(ULONG64)PdPhysical, DEFAULT_PAGE_SIZE);
        *Pdpe = MakeEntry(PdPhysical, FLAG_PRESENT | FLAG_RW);
        mfence();
    }

    ULONG64* Pd = (ULONG64*)(ULONG64)EntryAddress(*Pdpe);

    if (size == LARGE_PAGE_SIZE)
    {
        if ((paddr & (LARGE_PAGE_SIZE - 1)) != 0 || (vaddr & (LARGE_PAGE_SIZE - 1)) != 0)
        {
            return EFI_INVALID_PARAMETER;
        }
        Pd[pd_index] = MakeEntry(paddr, (flags | FLAG_PS));
        mfence();
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
    if ((PhysStart & DEFAULT_PAGE_SIZE - 1) || (PhysEnd & DEFAULT_PAGE_SIZE - 1) || (PhysStart >= PhysEnd))
    {
        return EFI_INVALID_PARAMETER;
    }

    ULONG64 Phys = PhysStart;
    EFI_STATUS Status;
    while (Phys < PhysEnd)
    {
        ULONG64 Remaining = PhysEnd - Phys;
        ULONG64 VA = DIRECT_MAP_BASE + Phys;

        if (((Phys | VA) & (LARGEST_PAGE_SIZE - 1)) == 0 && Remaining >= LARGEST_PAGE_SIZE)
        {
            Status = MapLargePage(VA, Phys, LARGEST_PAGE_SIZE, FLAG_PRESENT | FLAG_RW | FLAG_NX);
            if (EFI_ERROR(Status))
            {
                return Status;
            }
            Phys += LARGEST_PAGE_SIZE;
        }
        else if (((Phys | VA) & (LARGE_PAGE_SIZE - 1)) == 0 && Remaining >= LARGE_PAGE_SIZE)
        {
            Status = MapLargePage( VA, Phys, LARGE_PAGE_SIZE, FLAG_PRESENT | FLAG_RW | FLAG_NX);
            if (EFI_ERROR(Status))
            {
                return Status;
            }
            Phys += LARGE_PAGE_SIZE;
        }
        else
        {
            Status = MapPage(VA, Phys, FLAG_PRESENT | FLAG_RW | FLAG_NX);
            if (EFI_ERROR(Status))
            {
                return Status;
            }
            Phys += DEFAULT_PAGE_SIZE;
        }
    }
    //ReloadCR3(); // Flush the TLB
    return EFI_SUCCESS;
}

EFI_STATUS
BLAPI
MapKernel(
    UINT64 KernelPhys, 
    UINT64 KernelBase
)
{

    EFI_IMAGE_DOS_HEADER* ImageDosHeader = (EFI_IMAGE_DOS_HEADER*)KernelPhys;
    EFI_IMAGE_NT_HEADERS* nt = (EFI_IMAGE_NT_HEADERS*)(KernelPhys + ImageDosHeader->e_lfanew);

    UINT64 hdr = nt->OptionalHeader.SizeOfHeaders;
    for (UINT64 off = 0; off < hdr; off += 0x1000) 
    {
        EFI_STATUS s = MapPage(KernelBase + off, KernelPhys + off, FLAG_PRESENT | FLAG_NX);
        if (EFI_ERROR(s))
        {
            return s;
        }
    }

    EFI_IMAGE_SECTION_HEADER* sec = EFI_IMAGE_FIRST_SECTION(nt);
    for (UINT16 i = 0; i < nt->FileHeader.NumberOfSections; ++i) 
    {
        UINT64 vaddr = KernelBase + sec[i].VirtualAddress;
        UINT64 paddr = KernelPhys + sec[i].VirtualAddress;
        UINT64 vsize = (sec[i].Misc.VirtualSize + 0xFFF) & ~0xFFFULL;

        UINT64 flags = FLAG_PRESENT;
        BOOLEAN is_code = (sec[i].Characteristics & EFI_IMAGE_SCN_CNT_CODE) != 0;
        BOOLEAN is_write = (sec[i].Characteristics & EFI_IMAGE_SCN_MEM_WRITE) != 0;
        BOOLEAN is_exec = (sec[i].Characteristics & EFI_IMAGE_SCN_MEM_EXECUTE) != 0;

        if (is_write) flags |= FLAG_RW;
        if (!is_exec) flags |= FLAG_NX; // NX for non-exec

        for (UINT64 off = 0; off < vsize; off += 0x1000)
        {
            EFI_STATUS s = MapPage(vaddr + off, paddr + off, flags);
            if (EFI_ERROR(s))
            {
                return s;
            }
        }
    }

    return EFI_SUCCESS;
}

VOID
BLAPI
WalkPhysical(
    ULONG64 Phys
)
{

}

#endif
