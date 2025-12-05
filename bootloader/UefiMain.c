#include "control.h"
#include "filesystem.h"
#include "image.h"
#include "pagemanager.h"
#include "special.h"
#include "status.h"

typedef struct _BOOT_INFO
{
    ULONG64 DirectMapBase;
    ULONG64 Pml4Physical;
} BOOT_INFO, * PBOOT_INFO;

CHAR8* gEfiCallerBaseName = "Scouse Systems";
const UINT32 _gUefiDriverRevision = 0x0;

EFI_STATUS
BLAPI
InitalSetup( EFI_HANDLE ImageHandle );

/**
 * @brief Needed for VisualUefi for some reason?
 */
EFI_STATUS
EFIAPI
UefiUnload( EFI_HANDLE ImageHandle )
{
    return EFI_SUCCESS;
};

EFI_STATUS
BLAPI
FindExeFile( _In_ PCWSTR FileName, _Inout_ PBL_LDR_LOADED_IMAGE_INFO FileInfo )
{
    getc();
    if( EFI_ERROR( BlInitFileSystem( ) ) )
    {
        DBG_ERROR( BlGetLastFileError( ), L"File system init failed\n" );
        getc( );
        return EFI_NOT_FOUND;
    }

    if( !EFI_ERROR( BlGetRootDirectory( NULL ) ) )
    {
        BlListAllFiles( );
    }
    else
    {
        if( EFI_ERROR( FILE_SYSTEM_STATUS ) )
        {
            DBG_ERROR( BlGetLastFileError( ),
                       L"Failed to get root directory of current FS\n" );
        }
    }

    DBG_INFO( L"\nLooking for %s file pointer AHHH\n", FileName );
    EFI_FILE_PROTOCOL* File = NULL;
    if( !EFI_ERROR( BlFindFile( FileName, &File ) ) )
    {

        CHAR16* Buffer;
        if( BlGetFileName( File, &Buffer ) )
        {
            DBG_INFO( L"Got the file -> %s\n\n", Buffer );
        }
        FreePool( Buffer );
    }
    else
    {
        DBG_ERROR( BlGetLastFileError(), L"Failed to find %s file pointer\n", FileName);
        return EFI_LOAD_ERROR;
    }

    BlGetRootDirectory( NULL );

    BlLdrLoadPEImage64( FileName, FileInfo );

    return EFI_SUCCESS;
};



EFI_STATUS
MappingExists(
    ULONG64 Pml4,
    ULONG64 VirtualAddress
)
{
    ULONG64 Pml4Index = PML4_INDEX( VirtualAddress );
    ULONG64 PdptIndex = PDPT_INDEX( VirtualAddress );
    ULONG64 PdIndex   = PD_INDEX( VirtualAddress );
    ULONG64 PtIndex   = PT_INDEX( VirtualAddress );

    ULONG64* Pml4t = (ULONG64*)Pml4;
    ULONG64* Pdpt;
    ULONG64* Pd;
    ULONG64* Pt;

    if( !( Pml4t[ Pml4Index ] & PAGE_FLAG_PRESENT ) )
    {
        return EFI_ABORTED;
    }

    Pdpt = (ULONG64*)(Pml4t[ Pml4Index ] & 0x000FFFFFFFFFF000ULL );

    if( !( Pdpt[ PdptIndex ] & PAGE_FLAG_PRESENT ) )
    {
        return EFI_ABORTED;
    }

    Pd = ( ULONG64* )( Pdpt[ PdptIndex ] & 0x000FFFFFFFFFF000ULL );

    if( !( Pd[ PdIndex ] & PAGE_FLAG_PRESENT ) )
    {
        return EFI_ABORTED;
    }

    Pt = ( ULONG64* )( Pd[ PdIndex ] & 0x000FFFFFFFFFF000ULL );

    if( !( Pt[ PtIndex ] & PAGE_FLAG_PRESENT ) )
    {
        return EFI_ABORTED;
    }
    
    return EFI_SUCCESS;
}

/**
 * @brief The entry point for the UEFI application.
 *
 * @param[in] EFI_HANDLE        The image handle of the UEFI application
 * @param[in] EFI_SYSTEM_TABLE* The system table of the UEFI application
 *
 * @return EFI_STATUS - The status of the UEFI application (useful if driver
 * loads this app)
 */
EFI_STATUS
EFIAPI
UefiMain( EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable )
{


    EFI_STATUS Status;
    Status = InitalSetup( ImageHandle );

    if( EFI_ERROR( Status ) )
    {
        DBG_ERROR( Status, L"Initial setup failed\n" );
        getc( );
        return Status;
    }

    BL_LDR_LOADED_IMAGE_INFO KernelImage = { 0 };
    Status = FindExeFile( L"kernel.exe", &KernelImage );

    if( EFI_ERROR( Status ) )
    {
        DBG_ERROR( Status, L"Failed to find image\n" );
        getc( );
        return Status;
    }

    Print( L"VirtualBase -> %p, Base -> %p , Entry %p\n", KernelImage.VirtualBase, KernelImage.Base, KernelImage.EntryPoint  );

    //
    // now we need to get details of memory, luckily efi provides this to us.
    // with memory map we are able to get what physical memory is not in use.
    // memory can be used by devices, firmware, etc.
    //
    BL_EFI_MEMORY_MAP SystemMemoryMap = { 0 };

    // get size of memory map
    EFI_STATUS MemMap = gBS->GetMemoryMap( &SystemMemoryMap.MapSize,
                                           SystemMemoryMap.Descriptor,
                                           &SystemMemoryMap.Key,
                                           &SystemMemoryMap.DescriptorSize,
                                           &SystemMemoryMap.Version );

    if( MemMap != EFI_BUFFER_TOO_SMALL )
    {
        DBG_ERROR( MemMap, L"Failed init Memory Map" );
        getc( );
        return 1;
    }

    // allocate memory for memory map
    SystemMemoryMap.MapSize += 2 * SystemMemoryMap.DescriptorSize;
    SystemMemoryMap.Descriptor = AllocateZeroPool( SystemMemoryMap.MapSize );

    // get memory map
    MemMap = gBS->GetMemoryMap( &SystemMemoryMap.MapSize,
                                SystemMemoryMap.Descriptor,
                                &SystemMemoryMap.Key,
                                &SystemMemoryMap.DescriptorSize,
                                &SystemMemoryMap.Version );

    if( EFI_ERROR( MemMap ) )
    {
        DBG_ERROR( MemMap, L"Failed get Memory Map" );
        getc( );
        return 1;
    }

    ULONG64 NumberOfDescriptors =
        SystemMemoryMap.MapSize / SystemMemoryMap.DescriptorSize;

    ULONG64 MaxAddress = 0;

    EFI_MEMORY_DESCRIPTOR* Desc = SystemMemoryMap.Descriptor;

#define SsGetNextDescriptor(desc, size)                                        \
  ((EFI_MEMORY_DESCRIPTOR*)(((UINT8*)(desc)) + size))

    //
    // we will get the highest memory address that is available to us
    //
    for( ULONG64 i = 0; i < NumberOfDescriptors; i++ )
    {
        switch( Desc->Type )
        {
            case EfiConventionalMemory:
            case EfiPersistentMemory:
            {
                ULONG64 End =
                    Desc->PhysicalStart + ( Desc->NumberOfPages * DEFAULT_PAGE_SIZE );
                if( End > MaxAddress )
                {
                    MaxAddress = End;
                }
                break;
            }
            default:
            {
                break;
            }
        }
        Desc = SsGetNextDescriptor( Desc, SystemMemoryMap.DescriptorSize );
    }

    // number of physical pages
    SsPfnCount = PHYSICAL_TO_PFN( MaxAddress );

    //
    // allocate memory for PFN entries
    //
    ULONG64 PfnSize = SsPfnCount * sizeof( PFN_ENTRY );
    ULONG64 PagesNeeded = ( PfnSize + DEFAULT_PAGE_SIZE - 1 ) / DEFAULT_PAGE_SIZE;
    ULONG64 PfnBase = 0;

    EFI_STATUS PfnAlloc = gBS->AllocatePages(
        AllocateAnyPages, EfiBootServicesData, PagesNeeded, &PfnBase );

    if( EFI_ERROR( PfnAlloc ) )
    {
        DBG_ERROR( PfnAlloc, L"Failed allocating pfn base\n" );
        getc( );
        return 0;
    }

    SsPfn = ( PFN_ENTRY* )PfnBase;

    // for now set all physical pages to reserved
    for( ULONG64 pfn = 0; pfn < SsPfnCount; pfn++ )
    {
        SsPfn[ pfn ].State = Reserved;
        SsPfn[ pfn ].Ref = 0;
        SsPfn[ pfn ].Offset = 0xffffff;
    }

    SsPfnFreeHead = 0xffffff;

    FreePool( SystemMemoryMap.Descriptor );

    // get size of memory map
    MemMap = gBS->GetMemoryMap( &SystemMemoryMap.MapSize,
                                           SystemMemoryMap.Descriptor,
                                           &SystemMemoryMap.Key,
                                           &SystemMemoryMap.DescriptorSize,
                                           &SystemMemoryMap.Version );

    if( MemMap != EFI_BUFFER_TOO_SMALL )
    {
        DBG_ERROR( MemMap, L"Failed init Memory Map" );
        getc( );
        return 1;
    }

    // allocate memory for memory map
    SystemMemoryMap.MapSize += 2 * SystemMemoryMap.DescriptorSize;
    SystemMemoryMap.Descriptor = AllocateZeroPool( SystemMemoryMap.MapSize );

    // get memory map
    MemMap = gBS->GetMemoryMap( &SystemMemoryMap.MapSize,
                                SystemMemoryMap.Descriptor,
                                &SystemMemoryMap.Key,
                                &SystemMemoryMap.DescriptorSize,
                                &SystemMemoryMap.Version );

    if( EFI_ERROR( MemMap ) )
    {
        DBG_ERROR( MemMap, L"Failed get Memory Map" );
        getc( );
        return 1;
    }

    NumberOfDescriptors =
        SystemMemoryMap.MapSize / SystemMemoryMap.DescriptorSize;

    Desc = SystemMemoryMap.Descriptor;

    //
    // iterate through memory map and set physical pages to free, that are free
    // anyways.
    //
    for( ULONG64 i = 0; i < NumberOfDescriptors; i++ )
    {
        ULONG64 Start = Desc->PhysicalStart;
        ULONG64 PageCount = Desc->NumberOfPages;
        ULONG64 End = Start + PageCount * DEFAULT_PAGE_SIZE;

        switch( Desc->Type )
        {
            // realistically we do not care about firmware memory anymore.
            // Do not include runtimeservices, MMIO or other reserved memory as we are
            // not mapping them.
            case EfiConventionalMemory:
            case EfiPersistentMemory:
            // case EfiBootServicesCode:
            // case EfiBootServicesData:
            {
                ULONG64 StartPFN = PHYSICAL_TO_PFN( Start );
                ULONG64 EndPFN = PHYSICAL_TO_PFN( End );

                for( ULONG64 PFN = StartPFN; PFN < EndPFN; PFN++ )
                {
                    SsPfn[ PFN ].State = Free;
                    SsPfn[ PFN ].Ref = 0;
                    SsPfn[ PFN ].Offset = ( UINT32 )SsPfnFreeHead;
                    SsPfnFreeHead = PFN;
                }
                // Print(L"Free data to use by pfn!!!\n");
                break;
            }

            default:
            {
                break;
            }
        }
        Desc = SsGetNextDescriptor( Desc, SystemMemoryMap.DescriptorSize );
    }

    DBG_INFO( L"PFN free head -> %p\n", SsPfnFreeHead );
    DBG_INFO( L"PFN count -> %d\n", SsPfnCount );

    ULONG64 Pml4Physical = SsPagingInit( );
    if( !Pml4Physical )
    {
        DBG_ERROR( L"Failed to allogcate Pml4 a physical address" );
        getc( );
        return 1;
    }

    DBG_INFO(
        L"PML4 Physical -> %p, MaxAddress -> %p\n", Pml4Physical, MaxAddress );

    DirectMapRange( 0, MaxAddress );

    __writecr3( Pml4Physical );

    DBG_INFO(L"Has not crashed!\n");


    ULONG64 NewStack;

    AllocatePage( &NewStack );

    EFI_STATUS St = MapPage( KERNEL_VA_STACK, NewStack, PAGE_FLAG_PRESENT | PAGE_FLAG_RW  );
    if (EFI_ERROR(St))
    {
        DBG_ERROR(St, L"Failed\n");
    }

    ULONG64 CodePage;

    AllocatePage( &CodePage );

    ULONG64 HostCode = ALIGN_PAGE( &__HostCode );

    CopyMem( ( PVOID )CodePage, ( PVOID )HostCode, DEFAULT_PAGE_SIZE );

    MapPage( KERNEL_VA_BASE, CodePage, PAGE_FLAG_PRESENT | PAGE_FLAG_RW );

    if( EFI_ERROR( MappingExists( Pml4Physical, KERNEL_VA_BASE ) ) )
    {
        DBG_ERROR( L"Error", L"Code page doesnt exit\n" );
    }

    if( EFI_ERROR( MappingExists( Pml4Physical, KERNEL_VA_STACK ) ) )
    {
        DBG_ERROR( L"Error", L"Stack page doesnt exit\n" );
    }

    ULONG64 SwitchPage = ALIGN_PAGE( &__switchcr3 );
    MapPage( SwitchPage, SwitchPage, PAGE_FLAG_PRESENT | PAGE_FLAG_RW );

    if( EFI_ERROR( MappingExists( Pml4Physical, SwitchPage ) ) )
    {
        DBG_ERROR( L"Error", L"Switch page doesnt exit\n" );
    }

    Print( L"SwitchPage->%p, Switchaddr->%p\n", SwitchPage, &__switchcr3 );

    ULONG64 start = __readtscserial( );

    EFI_STATUS st = gBS->Stall( 1000000 );
    if( EFI_ERROR( st ) )
        return st;

    ULONG64 end = __readtscserial( );

    ULONG64 frequency = end - start;

    Print( L"Cycles per sec -> %llu\n", frequency );

    MapKernel(  KernelImage.Base, KernelImage.VirtualBase );
    __writecr3( Pml4Physical );

    typedef int (*KernelCall)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, PBOOT_INFO);
    KernelCall KEntry = (KernelCall)(KernelImage.VirtualBase + KernelImage.EntryPoint);

   /* __switchcr3( Pml4Physical & ~0xFFF, KERNEL_VA_STACK_TOP, KERNEL_VA_BASE + ( KernelImage.EntryPoint ) );*/

    int ReturnValue = KEntry( gST->ConOut, NULL );

    Print( L"Kernel returned %d", ReturnValue );


    getc( );



    return EFI_SUCCESS;
}

EFI_STATUS
BLAPI
InitalSetup(
    EFI_HANDLE ImageHandle
)
{
    EFI_LOADED_IMAGE* LoadedIamge = NULL;
    EFI_STATUS err = gBS->HandleProtocol(
        ImageHandle, &gEfiLoadedImageProtocolGuid, &LoadedIamge );

    if( EFI_ERROR( err ) )
    {
        return err;
    }

    DBG_INFO( L"handle-> %p", LoadedIamge->ImageBase );

    gST->ConOut->ClearScreen( gST->ConOut );

    EFI_TIME time;
    gRT->GetTime( &time, NULL );

    Print( L"%02d/%02d/%04d ----- %02d:%02d:%0d.%d\r\n",
           time.Day,
           time.Month,
           time.Year,
           time.Hour,
           time.Minute,
           time.Second,
           time.Nanosecond );

    return EFI_SUCCESS;
};
