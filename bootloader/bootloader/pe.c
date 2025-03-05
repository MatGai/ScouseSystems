#include "pe.h"

BOOLEAN
PeIsValidImage(
	_In_ PBYTE PE
)
{
	if (PE == NULL)
	{
		return FALSE;
	}

	EFI_IMAGE_DOS_HEADER* Dos = (EFI_IMAGE_DOS_HEADER*)PE;
	if (Dos->e_magic != EFI_IMAGE_DOS_SIGNATURE)
	{
		return FALSE;
	}

	EFI_IMAGE_NT_HEADERS* NT = (EFI_IMAGE_NT_HEADERS*)(PE + Dos->e_lfanew);
	if (NT->Signature != EFI_IMAGE_NT_SIGNATURE)
	{
		return FALSE;
	}

	return TRUE;
}