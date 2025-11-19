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

    DBG_INFO( L"\nLooking for %s file pointer\n", FileName );
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
        DBG_ERROR( L"[File]", L"Failed to find %s file pointer\n", FileName );
        getc( );
        return EFI_LOAD_ERROR;
    }

    BlGetRootDirectory( NULL );

    BlLdrLoadPEImage64( FileName, FileInfo );
};

EFI_STATUS
BLAPI
DumpPage( ULONG64 Address, // base virtual address
          ULONG64 Size // number of 8-byte entries
)
{
    ULONG64 start = Address;
    ULONG64 end = Address + ( Size * sizeof( ULONG64 ) );

    for( ULONG64 addr = start; addr < end; addr += 0x10 ) // 16 bytes per line
    {
        // First 8 bytes
        ULONG64* p0 = ( ULONG64* )( UINTN )addr;
        ULONG64 v0 = *p0;

        Print( L"0x%p -> 0x%p", addr, v0 );

        // Second 8 bytes (only if still inside range)
        if( addr + 0x8 < end )
        {
            ULONG64* p1 = ( ULONG64* )( UINTN )( addr + 0x8 );
            ULONG64 v1 = *p1;

            Print( L" | 0x%p -> 0x%p", addr + 0x8, v1 );
        }

        Print( L"\n" );
    }

    return EFI_SUCCESS;
};

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

    DumpPage( gPML4, 2 );


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

    BlDbgBreak( );

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
