#include "filesystem.h"

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
     
    gST->ConOut->ClearScreen( gST->ConOut );

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

    Print(L"Looking for 'kernel.exe' file pointer\n");
    EFI_FILE_PROTOCOL* File = NULL;
    if (BlFindFile(L"kernel.exe", &File))
    {
        CHAR16* Buffer;
        if (BlGetFileName(File, &Buffer))
        {
            Print(L"Got the file -> %s\n",Buffer);
        }
        FreePool(Buffer);
    }

    getc();

    if (BlGetRootDirectoryByIndex(FS1, NULL))
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

    Print(L"Looking for 'kernel.exe' file pointer\n");
    if (BlFindFile(L"kernel.exe", &File))
    {
        CHAR16* Buffer;
        if (BlGetFileName(File, &Buffer))
        {
            Print(L"Got the file -> %s\n", Buffer);
        }
        FreePool(Buffer);
    }


#ifdef _DEBUG_
    Print(L"EFI System Table Info\r\n   Signature: 0x%lx\r\n   UEFI Revision: 0x%08x\r\n   Header Size: %u Bytes\r\n   CRC32: 0x%08x\r\n   Reserved: 0x%x\r\n", gST->Hdr.Signature, gST->Hdr.Revision, gST->Hdr.HeaderSize, gST->Hdr.CRC32, gST->Hdr.Reserved);
#else
    Print(L"EFI System Table Info\r\n   Signature: 0x%lx\r\n   UEFI Revision: %u.%u", gST->Hdr.Signature, gST->Hdr.Revision >> 16, (gST->Hdr.Revision & 0xFFFF) / 10);
    if ((gST->Hdr.Revision & 0xFFFF) % 10)
    {
        Print(L".%u\r\n", (gST->Hdr.Revision & 0xFFFF) % 10); // UEFI major.minor version numbers are defined in BCD (in a 65535.65535 format) and are meant to be displayed as 2 digits if the minor ones digit is 0. Sub-minor revisions are included in the minor number. See the "EFI_TABLE_HEADER" section in any UEFI spec.
        // The spec also states that minor versions are limited to a max of 99, even though they get to have a whole 16-bit number.
    }
    else
    {
        Print(L"\r\n");
    }
#endif

    Print(L"   Firmware Vendor: %s\r\n   Firmware Revision: 0x%08x\r\n", gST->FirmwareVendor, gST->FirmwareRevision);

    getc();

    return EFI_SUCCESS;
}