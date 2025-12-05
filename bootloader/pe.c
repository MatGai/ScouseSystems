#include "pe.h"

BOOLEAN
PeIsValidImage(
    _In_ PBYTE PE
)
{
    if( PE == NULL )
    {
        return FALSE;
    }


    EFI_IMAGE_DOS_HEADER* Dos = ( EFI_IMAGE_DOS_HEADER* )PE;
    if( Dos->e_magic != EFI_IMAGE_DOS_SIGNATURE )
    {
        return FALSE;
    }

    EFI_IMAGE_NT_HEADERS* NT = ( EFI_IMAGE_NT_HEADERS* )( PE + Dos->e_lfanew );
    if( NT->Signature != EFI_IMAGE_NT_SIGNATURE )
    {
        return FALSE;
    }

    if( NT->FileHeader.Machine != IMAGE_FILE_MACHINE_X64 && NT->OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC )
    {
        return FALSE;
    }

    //if (NT->OptionalHeader.Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) 
    //{
    //	return FALSE;
    //}

    return TRUE;
}