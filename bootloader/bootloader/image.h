#ifndef IMAGE_H
#define IMAGE_H

#include "boot.h"
#include <Library/BaseMemoryLib.h>
#include "pe.h"

typedef struct _BL_LDR_LOADED_IMAGE_INFO
{
	ULONG64 Base;
	ULONG64 Size;
	ULONG64 VirtualBase;
	ULONG64 EntryPoint;


} BL_LDR_LOADED_IMAGE_INFO, *PBL_LDR_LOADED_IMAGE_INFO;

typedef struct _BL_LDR_FILE_IMAGE
{
	PBYTE File;
	UINTN FileSize;

} BL_LDR_FILE_IMAGE, *PBL_LDR_FILE_IMAGE;

EFI_STATUS
BLAPI
BlLdrLoadPEImageFile(
	_In_ PCWSTR ImagePath,
	_Inout_ PBL_LDR_FILE_IMAGE FileImage
);

EFI_STATUS
BLAPI
BlLdrAllocatePEImagePages(
	_In_ PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE* ImagePages,
	_Out_ EFI_PHYSICAL_ADDRESS* ImagePagesPhysical
);

EFI_STATUS
BLAPI
BlLdrAlignFileImage(
	_In_    PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE Image
);

EFI_STATUS
BLAPI
BlLdrImageRelocation(
	_In_    PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE Image,
	_In_ ULONG64
);

EFI_STATUS
BLAPI
BlLdrLoadPEImage64(
	_In_ PCWSTR ImagePath,
	_Inout_ PBL_LDR_LOADED_IMAGE_INFO LoadedImageInfo
);


#endif // !IMAGE_H