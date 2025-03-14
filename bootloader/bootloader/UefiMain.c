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
* @param[in] EFI_HANDLE        - The image handle of the UEFI application
* @param[in] EFI_SYSTEM_TABLE* - The system table of the UEFI application
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

    Print(L"%02d/%02d/%04d ----- %02d:%02d:%0d.%d\r\n", time.Month, time.Day, time.Year, time.Hour, time.Minute, time.Second, time.Nanosecond);

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

    DEBUG_INFO(L"starting load kernel\n");

    BlLdrLoadPEImage64(L"kernel.exe", &FileInfo);

    typedef int(__cdecl* KernelEntry)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut);
    KernelEntry EntryPoint = FileInfo.EntryPoint;
    int ret = EntryPoint(gST->ConOut);

    Print(L"EntryPoint returned %d\n", ret);

    getc();

    BL_EFI_MEMORY_MAP SystemMemoryMap;

    EFI_STATUS MemMap = gBS->GetMemoryMap(&SystemMemoryMap.MapSize, SystemMemoryMap.Descriptor, SystemMemoryMap.Key, SystemMemoryMap.DescriptorSize, SystemMemoryMap.Version);

    if( EFI_ERROR( MemMap ))
    {
        getc();
        DEBUG_ERROR(MemMap, L"Failed Memory Map");
        return 1;
    }

    SystemMemoryMap.MapSize += 2 * SystemMemoryMap.DescriptorSize;
    SystemMemoryMap.MapSize = AllocateZeroPool(SystemMemoryMap.MapSize);

    MemMap = gBS->GetMemoryMap(&SystemMemoryMap.MapSize, SystemMemoryMap.Descriptor, &SystemMemoryMap.Key, &SystemMemoryMap.DescriptorSize, &SystemMemoryMap.Version);

    if (EFI_ERROR(MemMap))
    {
        getc();
        DEBUG_ERROR(MemMap, L"Failed Memory Map");
        return 1;
    }

    return EFI_SUCCESS;
}