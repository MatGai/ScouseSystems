#include "filesystem.h"
#include "image.h"
#include "pagemanager.h"

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
}

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
    EFI_LOADED_IMAGE* LoadedIamge = NULL;
    EFI_STATUS err = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, &LoadedIamge);

    if (EFI_ERROR(err))
    {
        return 0;
    }

    Print(L"handle-> %p", LoadedIamge->ImageBase);
    //__debugbreak();

    gST->ConOut->ClearScreen(gST->ConOut);

    EFI_TIME time;
    gRT->GetTime(&time, NULL);

    Print(L"%02d/%02d/%04d ----- %02d:%02d:%0d.%d\r\n", time.Day, time.Month, time.Year, time.Hour, time.Minute, time.Second, time.Nanosecond);

    if (!BlInitFileSystem())
    {
        return 1;
    }

    if (BlGetRootDirectory(NULL))
    {
        BlListAllFiles();
    }
    else
    {
        if (EFI_ERROR(FILE_SYSTEM_STATUS))
        {
            Print(L"[ %r ] Failed to get root directory of current FS\n", BlGetLastFileError());
        }
    }

    getc();

    Print(L"\nLooking for 'kernel.exe' file pointer\n");
    EFI_FILE_PROTOCOL* File = NULL;
    if (BlFindFile(L"kernel.exe", &File))
    {
        CHAR16* Buffer;
        if (BlGetFileName(File, &Buffer))
        {
            Print(L"Got the file -> %s\n\n", Buffer);
        }
        FreePool(Buffer);
    }

    getc();

    BlGetRootDirectory(NULL);

    BL_LDR_LOADED_IMAGE_INFO FileInfo;

    DBG_INFO(L"starting load kernel\n");

    BlLdrLoadPEImage64(L"kernel.exe", &FileInfo);

    typedef int(__cdecl* KernelEntry)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut);
    KernelEntry EntryPoint = (KernelEntry)FileInfo.EntryPoint;
    int ret = EntryPoint(gST->ConOut);

    Print(L"EntryPoint returned %d\n", ret);
    Print(L"Kernel base %p\n", FileInfo.Base);
    getc();


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
        ULONG64 End = Desc->PhysicalStart + (Desc->NumberOfPages * DEFAULT_PAGE_SIZE);
        if (End > MaxAddress)
        {
            MaxAddress = End;
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
            case EfiConventionalMemory:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiPersistentMemory:
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

    Print(L"PFN free head -> %p\n", SsPfnFreeHead);
    Print(L"PFN count -> %d\n", SsPfnCount);

    getc();

    return EFI_SUCCESS;
}