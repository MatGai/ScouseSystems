#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include <Library/BaseMemoryLib.h>

#include "boot.h"
#include "general.h"
#include "pe.h"

#ifdef __cplusplus
extern "C" {

#endif

    void __switchcr3( unsigned long long, unsigned long long, unsigned long long );
    void __hostcode( void );

#ifdef __cplusplus
}
#endif

typedef enum _PFN_STATE
{
    Free, Allocated, Reserved
} PFN_STATE;

typedef struct _PFN_ENTRY
{
    PFN_STATE State;
    UINT32 Offset;
    UINT32 Ref;
} PFN_ENTRY, * PPFN_ENTRY;

PPFN_ENTRY SsPfn;
ULONG64 SsPfnCount;
ULONG64 SsPfnFreeHead;

#define PML4_INDEX(va) (((ULONG64)(va) >> 39) & 0x1FF)
#define PDPT_INDEX(va) (((ULONG64)(va) >> 30) & 0x1FF)
#define PD_INDEX(va) (((ULONG64)(va) >> 21) & 0x1FF)
#define PT_INDEX(va) (((ULONG64)(va) >> 12) & 0x1FF)

#define DEFAULT_PAGE_SIZE 0x1000
#define LARGE_PAGE_SIZE   0x200000
#define HUGE_PAGE_SIZE    0x40000000

#define PAGE_SHIFT 12
#define PFN_LIST_END 0xFFFFFF
                     
#define PFN_TO_PHYSICAL_SIZE(pfn, size) ((pfn) << size)
#define PFN_TO_PHYSICAL(pfn) (PFN_TO_PHYSICAL_SIZE(pfn, PAGE_SHIFT))

#define PHYSICAL_TO_PFN_SIZE(adr, size) ((adr) >> size)
#define PHYSICAL_TO_PFN(adr) (PHYSICAL_TO_PFN_SIZE(adr, PAGE_SHIFT))


#define LOW_MEMORY_START 0x0000000000000000ULL
#define LOW_MEMORY_END   0x00007FFFFFFFFFFFULL

#define HIGH_MEMORY_START 0xFFFF800000000000ULL
#define HIGH_MEMORY_END   0xFFFFFFFFFFFFFFFFULL

#define DIRECT_MAP_BASE HIGH_MEMORY_START

#define KERNEL_VA_BASE 0xFFFFFFFF80000000ULL
#define KERNEL_VA_STACK_TOP ( KERNEL_VA_BASE - 0x1000 - 0x10 )
#define KERNEL_VA_STACK ( KERNEL_VA_BASE - 0x2000 )


#define ALIGN_PAGE( Value )  ( (ULONG64)Value & ~0xFFFull )
#define PAGE_OFFSET( Value ) ( (ULONG64)Value & 0xFFFull  )

#define ALIGN_LARGE_PAGE( Value )  ( (ULONG64)Value & ~0xFFFFFull )
#define LARGE_PAGE_OFFSET( Value ) ( (ULONG64)Value & 0xFFFFFull  )

#define ALIGN_HUGE_PAGE( Value )  ( (ULONG64)Value & ~0xFFFFFFFull )
#define HUGE_PAGE_OFFSET( Value ) ( (ULONG64)Value & 0xFFFFFFFull  )


#define PAGE_FLAG_PRESENT ( 1ull << 0 )
#define PAGE_FLAG_RW      ( 1ull << 1 )
#define PAGE_FLAG_USER    ( 1ull << 2 )
#define PAGE_FLAG_PS      ( 1ull << 7 )
#define PAGE_FLAG_NX      ( 1ull << 63 )

// Makes a page entry with present and r/w and nx 
#define MAKE_PAGE_ENTRY( Entry, NextTablePhysical )  \
   (Entry) = ((NextTablePhysical) & 0x000FFFFFFFFFF000ull) | ( ( PAGE_FLAG_PRESENT | PAGE_FLAG_RW ) & 0xFFF0000000000FFFull );

// Makes a page point to a physical page with desired flags
#define MAKE_LEAF_ENTRY( Entry, PagePhysical, Flags ) \
   (Entry) = ((PagePhysical) & 0x000FFFFFFFFFF000ull) | (Flags & 0xFFF0000000000FFFull);

typedef struct _BL_EFI_MEMORY_MAP
{
    EFI_MEMORY_DESCRIPTOR* Descriptor;
    ULONG64 MapSize;
    ULONG64 Key;
    ULONG64 DescriptorSize;
    UINT32 Version;
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
    return ( VOID* )( DIRECT_MAP_BASE + PhysicalAddress );
}

ULONG64
BLAPI
SsGetFreePhysicalPage(
    VOID
)
{
    if( SsPfnFreeHead == PFN_LIST_END )
    {
        DBG_INFO( L"No Free memory!" );
        getc( );
        return 0;
    }

    ULONG64 PageBase = PFN_TO_PHYSICAL( SsPfnFreeHead );
    PFN_ENTRY* Entry = &SsPfn[ SsPfnFreeHead ];

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

    if( IndexPFN >= SsPfnCount )
    {
        DBG_INFO( L"Pfn too large\n" );
        getc( );
        return;
    }

    PFN_ENTRY* PFN = &SsPfn[ IndexPFN ];
    PFN->State = Free;
    PFN->Ref = 0;
    PFN->Offset = ( UINT32 )SsPfnFreeHead;
    SsPfnFreeHead = IndexPFN;
};

EFI_STATUS
BLAPI
AllocatePage(
    ULONG64* Pa
)
{
    ULONG64 FreePage = SsGetFreePhysicalPage( );
    if( !FreePage )
    {
        DBG_INFO( L"No free memory!" );
        getc( );
        return EFI_OUT_OF_RESOURCES;
    }
    EFI_STATUS st =
        gBS->AllocatePages( AllocateAddress, EfiLoaderData, 1, &FreePage );

    if( EFI_ERROR( st ) )
    {
        DBG_INFO( L"Failed to allocate pages!" );
        getc( );
        return st;
    }

    *Pa = FreePage;
    return EFI_SUCCESS;
}

// DirectPagingInit initializes our paging structures for the direct map.
// It allocates one page for the PML4 (level-4 table) and zeros it.
ULONG64
BLAPI
SsPagingInit(
    VOID
)
{
    ULONG64 FreePage;

    AllocatePage( &FreePage );

    if( !FreePage )
    {
        DBG_INFO( L"No free memory!" );
        getc( );
        return ( ULONG64 )NULL;
    }

    gPML4 = ( ULONG64* )( PVOID )FreePage;

    ZeroMem( gPML4, DEFAULT_PAGE_SIZE );

    return FreePage;
}

// MapPage maps a single 4KB page so that virtual address 'vaddr'
// maps to physical address 'paddr' with the specified 'flags' (for the PTE).
// This function walks the 4-level page table hierarchy, allocating lower-level
// tables on demand. It uses our global gPML4 (which is assumed to be already
// initialized).
EFI_STATUS
MapPage(
    ULONG64 VirtualAddress,
    ULONG64 PhysicalAddress,
    ULONG64 Flags
)
{
    // Decompose vaddr to get table indexs
    ULONG64 Pml4Index = PML4_INDEX( VirtualAddress );
    ULONG64 PdptIndex = PDPT_INDEX( VirtualAddress );
    ULONG64 PdIndex = PD_INDEX( VirtualAddress );
    ULONG64 PtIndex = PT_INDEX( VirtualAddress );

    ULONG64* Pml4t = gPML4;
    ULONG64* Pdpt  = NULL;
    ULONG64* Pdt   = NULL;
    ULONG64* Pt    = NULL;

    if( !( Pml4t[ Pml4Index ] & PAGE_FLAG_PRESENT ) )
    {
        ULONG64 PdptPhysical;
        AllocatePage( &PdptPhysical );
        if( !PdptPhysical )
        {
            return EFI_OUT_OF_RESOURCES;
        }
        MAKE_PAGE_ENTRY( Pml4t[ Pml4Index ], PdptPhysical );
    }

    Pdpt = ( ULONG64* )( Pml4t[ Pml4Index ] & 0x000FFFFFFFFFF000ull );

    if( !( Pdpt[ PdptIndex ] & PAGE_FLAG_PRESENT ) )
    {
        ULONG64 PdPhysical;
        AllocatePage( &PdPhysical );
        if( !PdPhysical )
        {
            return EFI_OUT_OF_RESOURCES;
        }
        MAKE_PAGE_ENTRY( Pdpt[ PdptIndex ], PdPhysical );
    }

    Pdt = ( ULONG64* )( Pdpt[ PdptIndex ] & 0x000FFFFFFFFFF000ull );

    if( !( Pdt[ PdIndex ] & PAGE_FLAG_PRESENT ) )
    {
        ULONG64 PtPhysical;
        AllocatePage( &PtPhysical );
        if( !PtPhysical )
        {
            return EFI_OUT_OF_RESOURCES;
        }
        MAKE_PAGE_ENTRY( Pdt[ PdIndex ], PtPhysical );
    }

    Pt = ( ULONG64* )( Pdt[ PdIndex ] & 0x000FFFFFFFFFF000ull );

    MAKE_LEAF_ENTRY( Pt[ PtIndex ], PhysicalAddress, Flags );

    return EFI_SUCCESS;
}

// MapLargePage maps a single 2MB or 1GB page, depending on 'size'.
// Size must be either LARGE_PAGE_SIZE or LARGEST_PAGE_SIZE
EFI_STATUS
MapLargePage( 
    ULONG64 VirtualAddress,
    ULONG64 PhysicalAddress, 
    ULONG64 flags )
{
    ULONG64 Pml4Index = PML4_INDEX( VirtualAddress );
    ULONG64 PdptIndex = PDPT_INDEX( VirtualAddress );
    ULONG64 PdIndex   = PD_INDEX( VirtualAddress );

    ULONG64* Pml4t = gPML4;
    ULONG64* Pdpt  = NULL;
    ULONG64* Pd    = NULL;

    if( !( Pml4t[ Pml4Index ] & PAGE_FLAG_PRESENT ) )
    {
        ULONG64 PdptPhysical;
        AllocatePage( &PdptPhysical );
        if( !PdptPhysical )
        {
            return EFI_OUT_OF_RESOURCES;
        }
        MAKE_PAGE_ENTRY( Pml4t[ Pml4Index ], PdptPhysical );
    }

   // Pdpt = ( ULONG64* )( Pml4t[ Pml4Index ] )


    return EFI_SUCCESS;
}

// DirectMapRange maps all physical memory from physStart to physEnd
// into the direct mapped region so that VA = DIRECT_MAP_BASE + PA.
// The caller must ensure that physStart and physEnd are page-aligned.
EFI_STATUS
DirectMapRange(
    ULONG64 PhysStart,
    ULONG64 PhysEnd
)
{
    //   if( ( PhysStart & DEFAULT_PAGE_SIZE - 1 ) ||
    //       ( PhysEnd & DEFAULT_PAGE_SIZE - 1 ) || ( PhysStart >= PhysEnd ) )
    //   {
    //       return EFI_INVALID_PARAMETER;
    //   }
    //
    //   ULONG64 Phys = PhysStart;
    //   EFI_STATUS Status;
    //   while( Phys < PhysEnd )
    //   {
    //       ULONG64 Remaining = PhysEnd - Phys;
    //       ULONG64 VA = DIRECT_MAP_BASE + Phys;
    //
    //       if( ( ( Phys | VA ) & ( HUGE_PAGE_SIZE - 1 ) ) == 0 &&
    //           Remaining >= HUGE_PAGE_SIZE )
    //       {
    //           Status = MapLargePage( VA, Phys, HUGE_PAGE_SIZE,
    //                                  FLAG_PRESENT | FLAG_RW | FLAG_NX );
    //           if( EFI_ERROR( Status ) )
    //           {
    //               return Status;
    //           }
    //           Phys += HUGE_PAGE_SIZE;
    //       }
    //       else if( ( ( Phys | VA ) & ( LARGE_PAGE_SIZE - 1 ) ) == 0 &&
    //                Remaining >= LARGE_PAGE_SIZE )
    //       {
    //           Status = MapLargePage( VA, Phys, LARGE_PAGE_SIZE,
    //                                  FLAG_PRESENT | FLAG_RW | FLAG_NX );
    //           if( EFI_ERROR( Status ) )
    //           {
    //               return Status;
    //           }
    //           Phys += LARGE_PAGE_SIZE;
    //       }
    //       else
    //       {
    //           Status = MapPage( VA, Phys, FLAG_PRESENT | FLAG_RW | FLAG_NX );
    //           if( EFI_ERROR( Status ) )
    //           {
    //               return Status;
    //           }
    //           Phys += DEFAULT_PAGE_SIZE;
    //       }
    //   }
        // ReloadCR3(); // Flush the TLB
    return EFI_SUCCESS;
}

EFI_STATUS
BLAPI
UnmapPage(
    ULONG64 vaddr
)
{
    return EFI_SUCCESS;
}

EFI_STATUS
BLAPI
MapKernel(
    UINT64 KernelPhys,
    UINT64 KernelBase
)
{

    return EFI_SUCCESS;
}

#endif  // !PAGEMANAGER_H
