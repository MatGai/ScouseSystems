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

#define PML4_INDEX( Value ) ( ( (ULONG64)( Value ) >> 39 ) & 0x1FF )
#define PDPT_INDEX( Value ) ( ( (ULONG64)( Value ) >> 30 ) & 0x1FF )
#define PD_INDEX( Value ) ( ( (ULONG64)( Value ) >> 21 ) & 0x1FF )
#define PT_INDEX( Value ) ( ( (ULONG64)( Value ) >> 12 ) & 0x1FF )

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
#define PML4_HIGH_MEMORY_START 256 // PML4[ 256 ] = 0xFFFF_8000_0000_0000

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
#define PAGE_PFN_MASK     0x000FFFFFFFFFF000ull

#define HUGE_PAGE_MAPPING_INDEX 1
#define LARGE_PAGE_MAPPING_INDEX 2
#define PAGE_MAPPING_INDEX 3

// Makes a page entry with present and r/w and nx 
#define MAKE_PAGE_ENTRY( Entry, NextTablePhysical )  \
   ( Entry ) = ( ( NextTablePhysical ) & PAGE_PFN_MASK ) | ( ( PAGE_FLAG_PRESENT | PAGE_FLAG_RW ) & PAGE_PFN_MASK );

// Makes a page point to a physical page with desired flags
#define MAKE_LEAF_ENTRY( Entry, PagePhysical, Flags ) \
   ( Entry ) = ( ( PagePhysical ) & PAGE_PFN_MASK ) | ( Flags & PAGE_PFN_MASK );

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

EFI_STATUS
UefiMapTables(
    ULONG64* Pml4,
    ULONG64 VirtualAddress,
    ULONG64 PhysicalAddress,
    ULONG64 TableEnd,
    ULONG64 Flags
)
{
    if( TableEnd < 1 || TableEnd > 3 )
    {
        return EFI_INVALID_PARAMETER;
    }

    ULONG64 EntryIndex[0x4] = 
    {
        PML4_INDEX( VirtualAddress ),
        PDPT_INDEX( VirtualAddress ),
        PD_INDEX( VirtualAddress ),
        PT_INDEX( VirtualAddress )
    };

    ULONG64* Table = Pml4;

    for( UINT32 i = 0; i < TableEnd; i++ )
    {
        ULONG64* TableEntry = &Table[ EntryIndex[ i ] ];
        if( !( *TableEntry & PAGE_FLAG_PRESENT ) )
        {
            ULONG64 NextTablePhysical;
            AllocatePage( &NextTablePhysical );
            if( !NextTablePhysical )
            {
                return EFI_OUT_OF_RESOURCES;
            }

            MAKE_PAGE_ENTRY( *TableEntry, NextTablePhysical );
        }

        Table = ( ULONG64* )( *TableEntry & 0x000FFFFFFFFFF000ull );;
    }

    MAKE_LEAF_ENTRY( Table[ EntryIndex[ TableEnd ] ], PhysicalAddress, Flags );

    return EFI_SUCCESS;
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
    EFI_STATUS St = UefiMapTables(
        gPML4,
        VirtualAddress,
        PhysicalAddress,
        PAGE_MAPPING_INDEX,
        Flags
    );

    return St;
}

// MapLargePage maps a single 2MiB page
EFI_STATUS
MapLargePage(
    ULONG64 VirtualAddress,
    ULONG64 PhysicalAddress,
    ULONG64 Flags
)
{
    EFI_STATUS St = UefiMapTables(
        gPML4,
        VirtualAddress,
        PhysicalAddress,
        LARGE_PAGE_MAPPING_INDEX,
        Flags | PAGE_FLAG_PS
    );

    return St;
}

EFI_STATUS
MapHugePage(
    ULONG64 VirtualAddress,
    ULONG64 PhysicalAddress,
    ULONG64 Flags
)
{
    EFI_STATUS St = UefiMapTables( 
        gPML4, 
        VirtualAddress, 
        PhysicalAddress, 
        HUGE_PAGE_MAPPING_INDEX, 
        Flags | PAGE_FLAG_PS 
    );

    return St;
}

// DirectMapRange maps all physical memory from physStart to physEnd
// into the direct mapped region so that VA = DIRECT_MAP_BASE + PA.
// The caller must ensure that physStart and physEnd are page-aligned.
EFI_STATUS
DirectMapRange(
    ULONG64 PhysicalStart,
    ULONG64 PhysicalEnd
)
{
    if( PhysicalStart != ALIGN_PAGE( PhysicalStart ) || 
        PhysicalEnd != ALIGN_PAGE( PhysicalEnd )     || 
        PhysicalStart >= PhysicalEnd )
    {
        return EFI_INVALID_PARAMETER;
    }

    ULONG64 CurrentPhysical = PhysicalStart;    
    EFI_STATUS St;
    ULONG64 Flags = PAGE_FLAG_PRESENT | PAGE_FLAG_RW;

    while( CurrentPhysical < PhysicalEnd )
    {
        ULONG64 VirtualAddress = /*DIRECT_MAP_BASE + */CurrentPhysical;
        ULONG64 Remaining = PhysicalEnd - CurrentPhysical;

        if( Remaining >= HUGE_PAGE_SIZE  )
        {
            MapHugePage( VirtualAddress, CurrentPhysical, Flags );
            CurrentPhysical += HUGE_PAGE_SIZE;
            continue;
        }
        
        if ( Remaining >= LARGE_PAGE_SIZE )
        {
            MapLargePage( VirtualAddress, CurrentPhysical, Flags );
            CurrentPhysical += LARGE_PAGE_SIZE;
            continue;
        }

        MapPage( VirtualAddress, CurrentPhysical, Flags );
        CurrentPhysical += DEFAULT_PAGE_SIZE;
    }

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
    ULONG64 KernelPhys,
    ULONG64 KernelBase
)
{

    return EFI_SUCCESS;
}

#endif  // !PAGEMANAGER_H
