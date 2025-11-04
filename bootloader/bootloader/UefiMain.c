#include "filesystem.h"
#include "image.h"
#include "pagemanager.h"
#include "control.h"
#include "special.h"

typedef struct _BOOT_INFO
{
    ULONG64 DirectMapBase;
    ULONG64 Pml4Physical;
} BOOT_INFO, * PBOOT_INFO;

CHAR8* gEfiCallerBaseName = "Scouse Systems";
const UINT32 _gUefiDriverRevision = 0x0;

/**
* @brief Needed for VisualUefi for some reason?
*/
EFI_STATUS
EFIAPI
UefiUnload(
    EFI_HANDLE ImageHandle
)
{
    return EFI_SUCCESS;
};

EFI_STATUS
BLAPI
InitalSetup(
    EFI_HANDLE* ImageHandle
)
{
    EFI_LOADED_IMAGE* LoadedIamge = NULL;
    EFI_STATUS err = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, &LoadedIamge);

    if (EFI_ERROR(err))
    {
        return err;
    }

    DBG_INFO(L"handle-> %p", LoadedIamge->ImageBase);

    BlDbgBreak();

    gST->ConOut->ClearScreen(gST->ConOut);

    EFI_TIME time;
    gRT->GetTime(&time, NULL);

    Print(L"%02d/%02d/%04d ----- %02d:%02d:%0d.%d\r\n", time.Day, time.Month, time.Year, time.Hour, time.Minute, time.Second, time.Nanosecond);
    
    return EFI_SUCCESS;
};

/**
* @brief The entry point for the UEFI application.
* 
* @param[in] EFI_HANDLE        The image handle of the UEFI application
* @param[in] EFI_SYSTEM_TABLE* The system table of the UEFI application
* 
* @return EFI_STATUS - The status of the UEFI application (useful if driver loads this app)
*/
EFI_STATUS 
EFIAPI 
UefiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE* SystemTable
)
{

    EFI_STATUS Status;
    Status = InitalSetup(&ImageHandle);

    //DBG_ERROR(Status, L"Hello\n");
    DBG_INFO(L"Hello %s\n", "sir");
    getc();

    if (EFI_ERROR(Status))
    {
        DBG_ERROR(Status, L"Initial setup failed\n");
        getc();
        return Status;
    }



    if ( EFI_ERROR(BlInitFileSystem()) )
    {
        DBG_ERROR(BlGetLastFileError(), L"File system init failed\n");
        getc();
        return 1;
    }

    if ( !EFI_ERROR(BlGetRootDirectory(NULL)) )
    {
        BlListAllFiles();
    }
    else
    {
        if ( EFI_ERROR(FILE_SYSTEM_STATUS) )
        {
            DBG_ERROR(BlGetLastFileError(), L"Failed to get root directory of current FS\n");
        }
    }

    DBG_INFO(L"\nLooking for 'kernel.exe' file pointer\n");
    EFI_FILE_PROTOCOL* File = NULL;
    if (!EFI_ERROR( BlFindFile(L"kernel.exe", &File) ))
    {
        CHAR16* Buffer;
        if (BlGetFileName(File, &Buffer))
        {
            DBG_INFO(L"Got the file -> %s\n\n", Buffer);
        }
        FreePool(Buffer);
    }
    else
    {
        DBG_ERROR(L"Failed to find 'kernel.exe' file pointer\n");
        getc();
        return EFI_LOAD_ERROR;
    }

    BlGetRootDirectory(NULL);

    BL_LDR_LOADED_IMAGE_INFO FileInfo;

    DBG_INFO(L"Starting load kernel\n");

    BlLdrLoadPEImage64(L"kernel.exe", &FileInfo);

    typedef int(__cdecl* KernelEntry)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut, PBOOT_INFO BootInfo);
    KernelEntry EntryPoint = (KernelEntry)FileInfo.EntryPoint;

    DBG_INFO(L"Kernel Image base %p, entry point %p, va %p\n", FileInfo.Base, FileInfo.EntryPoint, FileInfo.VirtualBase);

    //
    // now we need to get details of memory, luckily efi provides this to us.
    // with memory map we are able to get what physical memory is not in use.
    // memory can be used by devices, firmware, etc.
    //
    BL_EFI_MEMORY_MAP SystemMemoryMap = { NULL };

    // get size of memory map
    EFI_STATUS MemMap = gBS->GetMemoryMap(&SystemMemoryMap.MapSize, SystemMemoryMap.Descriptor, &SystemMemoryMap.Key, &SystemMemoryMap.DescriptorSize, &SystemMemoryMap.Version);

    if( MemMap != EFI_BUFFER_TOO_SMALL )
    {   
        DBG_ERROR(MemMap, L"Failed init Memory Map");
        getc();
        return 1;
    }

    // allocate memory for memory map
    SystemMemoryMap.MapSize += 2 * SystemMemoryMap.DescriptorSize;
    SystemMemoryMap.Descriptor = AllocateZeroPool(SystemMemoryMap.MapSize);

    // get memory map
    MemMap = gBS->GetMemoryMap(&SystemMemoryMap.MapSize, SystemMemoryMap.Descriptor, &SystemMemoryMap.Key, &SystemMemoryMap.DescriptorSize, &SystemMemoryMap.Version);

    if (EFI_ERROR(MemMap))
    {
        DBG_ERROR(MemMap, L"Failed get Memory Map");
        getc();
        return 1;
    }

    ULONG64 NumberOfDescriptors = SystemMemoryMap.MapSize / SystemMemoryMap.DescriptorSize;

    ULONG64 MaxAddress = 0;

    EFI_MEMORY_DESCRIPTOR* Desc = SystemMemoryMap.Descriptor;

#define SsGetNextDescriptor( desc, size ) ( (EFI_MEMORY_DESCRIPTOR*)( ((UINT8*)(desc)) + size ) )

    //
    // we will get the highest memory address that is available to us
    //
    for (ULONG64 i = 0; i < NumberOfDescriptors; i++)
    {
        switch (Desc->Type)
        {
            case EfiConventionalMemory:
            {
                ULONG64 End = Desc->PhysicalStart + (Desc->NumberOfPages * DEFAULT_PAGE_SIZE);
                if (End > MaxAddress)
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
        Desc = SsGetNextDescriptor(Desc, SystemMemoryMap.DescriptorSize);
    }

    // number of physical pages
    SsPfnCount = PHYSICAL_TO_PFN(MaxAddress);
    
    //
    // allocate memory for PFN entries
    //
    ULONG64 PfnSize = SsPfnCount * sizeof( PFN_ENTRY );
    ULONG64 PagesNeeded = (PfnSize + DEFAULT_PAGE_SIZE - 1) / DEFAULT_PAGE_SIZE;
    ULONG64 PfnBase = 0;

    EFI_STATUS PfnAlloc = gBS->AllocatePages( AllocateAnyPages, EfiBootServicesData, PagesNeeded, &PfnBase );

    if (EFI_ERROR(PfnAlloc))
    {
        DBG_ERROR(PfnAlloc, L"Failed allocating pfn base\n");
        getc();
        return 0;
    }

    SsPfn = (PFN_ENTRY*)PfnBase;

    // for now set all physical pages to reserved
    for (ULONG64 pfn = 0; pfn < SsPfnCount; pfn++)
    {
        SsPfn[pfn].State = Reserved;
        SsPfn[pfn].Ref = 0;
        SsPfn[pfn].Offset = 0xffffff;
    }

    SsPfnFreeHead = 0xffffff;

    Desc = SystemMemoryMap.Descriptor;

    //
    // iterate through memory map and set physical pages to free, that are free anyways.
    //
    for( ULONG64 i = 0; i < NumberOfDescriptors; i++ )
    {
        ULONG64 Start = Desc->PhysicalStart;
        ULONG64 PageCount = Desc->NumberOfPages;
        ULONG64 End = Start + PageCount * DEFAULT_PAGE_SIZE;

        switch (Desc->Type)
        {
            // realistically we do not care about firmware memory anymore. 
            // Do not include runtimeservices, MMIO or other reserved memory as we are not mapping them.
            case EfiConventionalMemory:
            //case EfiBootServicesCode:
            //case EfiBootServicesData:
            {
                ULONG64 StartPFN = PHYSICAL_TO_PFN(Start);
                ULONG64 EndPFN = PHYSICAL_TO_PFN(End);

                for( ULONG64 PFN = StartPFN; PFN  < EndPFN; PFN++ )
                {
                    SsPfn[ PFN ].State = Free;
                    SsPfn[ PFN ].Ref = 0;
                    SsPfn[ PFN ].Offset = (UINT32)SsPfnFreeHead;
                    SsPfnFreeHead = (ULONG64)PFN;
                }
                //Print(L"Free data to use by pfn!!!\n");
                break;
            }

            default:
            {
                break;
            }
            
        }
        Desc = SsGetNextDescriptor( Desc, SystemMemoryMap.DescriptorSize );
    }

    DBG_INFO(L"PFN free head -> %p\n", SsPfnFreeHead);
    DBG_INFO(L"PFN count -> %d\n", SsPfnCount);

    ULONG64 Pml4Physical = SsPagingInit();
    if (!Pml4Physical)
    {
        DBG_ERROR(L"Failed to allogcate Pml4 a physical address");
        getc();
        return 1;
    }

    DBG_INFO(L"PML4 Physical -> %p, MaxAddress -> %p\n", Pml4Physical, MaxAddress);
    getc();

    EFI_STATUS MapStatus = DirectMapRange(0, MaxAddress);
    if (EFI_ERROR(MapStatus))
    {
        DBG_ERROR(MapStatus, L"Failed to map RAM\n");
        getc();
        return 1;
    }

    DBG_INFO(L"Direct mapped range 0x%p - 0x%p\n", DIRECT_MAP_BASE, MaxAddress);
    getc();

    EFI_STATUS s = MapKernel(FileInfo.Base, KERNEL_VA_BASE);
    DBG_ERROR(s);

    DBG_INFO(L"Kernel mapped to %p, entry rva %p\n", KERNEL_VA_BASE, KERNEL_VA_BASE + FileInfo.EntryPoint);
    DBG_INFO(L"CR3 set to %p\n", Pml4Physical);
    getc();
    
    BlDbgBreak();

    __writecr3(Pml4Physical);

    BOOT_INFO BootInfo = { DIRECT_MAP_BASE, Pml4Physical };

    typedef int(__cdecl* KernelEntry)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, PBOOT_INFO);
    KernelEntry Entry = (KernelEntry)(UINTN)(KERNEL_VA_BASE + FileInfo.EntryPoint);

    int ret = Entry(gST->ConOut, &BootInfo);

    DBG_INFO(L"EntryPoint returned %d\n", ret);
    DBG_INFO(L"Kernel base %p\n", FileInfo.Base);
    getc();

    return EFI_SUCCESS;
}