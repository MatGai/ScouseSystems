#ifndef IMAGE_H
#define IMAGE_H

#include "boot.h"
#include <Library/BaseMemoryLib.h>
#include "pe.h"

typedef struct _BL_LDR_LOADED_IMAGE_INFO
{
	ULONG64 Base;
	ULONG64 Size;

	ULONG64 EntryPoint;

} BL_LDR_LOADED_IMAGE_INFO, *PBL_LDR_LOADED_IMAGE_INFO;

typedef struct _BL_LDR_FILE_IMAGE
{
	PBYTE File;
	UINTN FileSize;

} BL_LDR_FILE_IMAGE, *PBL_LDR_FILE_IMAGE;

BL_STATUS
BLAPI
BlLdrLoadPEImageFile(
	_In_ PCWSTR ImagePath,
	_Inout_ PBL_LDR_FILE_IMAGE FileImage
);

BL_STATUS
BLAPI
BlLdrAllocatePEImagePages(
	_In_ PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE* ImagePages,
	_Out_ EFI_PHYSICAL_ADDRESS* ImagePagesPhysical
);

BL_STATUS
BLAPI
BlLdrAlignFileImage(
	_In_    PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE Image
);

BL_STATUS
BLAPI
BlLdrImageRelocation(
	_In_    PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE Image
);

BL_STATUS
BLAPI
BlLdrLoadPEImage64(
	_In_ PCWSTR ImagePath,
	_Inout_ PBL_LDR_LOADED_IMAGE_INFO LoadedImageInfo
);


#endif // !IMAGE_H