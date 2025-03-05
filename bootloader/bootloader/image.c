#include "image.h"
#include "pe.h"
#include "filesystem.h"

BL_STATUS
BLAPI
BlLdrLoadPEImageFile(
	_In_ CHAR8* ImagePath,
	_Inout_ PBL_LDR_FILE_IMAGE FileImageData
)
{
	if (ImagePath == NULL || FileImageData == NULL)
	{
		return BL_STATUS_INVALID_PARAMETER;
	}

	EFI_FILE_PROTOCOL* ImageFileHandle = NULL;
	if (BlFindFile(ImagePath, &ImageFileHandle) == FALSE)
	{
		return BL_STATUS_INVALID_PATH;
	}

	EFI_FILE_INFO* ImageFileInfo = NULL;
	if (BlGetFileInfo(ImageFileHandle, &ImageFileInfo) == FALSE)
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	UINTN FileImageSize = ImageFileInfo->FileSize;

	gBS->FreePool(ImageFileInfo);

	PBYTE FileImage = NULL;
	if (EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, FileImageSize, &FileImage)))
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	if (EFI_ERROR(ImageFileHandle->Read(ImageFileHandle, &FileImageSize, FileImage)))
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	FileImageData->File = FileImage;
	FileImageData->FileSize = FileImageSize;

	return BL_STATUS_OK;
}

BL_STATUS
BLAPI
BlLdrAllocatePEImagePages(
	_In_ PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE* ImagePages,
	_Inout_ EFI_PHYSICAL_ADDRESS* ImagePagesPhysical
)
{
	if (FileImage == NULL || ImagePagesPhysical == NULL || ImagePages == NULL)
	{
		return BL_STATUS_INVALID_PARAMETER;
	}

	PEFI_IMAGE_NT_HEADERS FileNTHeaders = EFI_IMAGE_NTHEADERS(FileImage->File);

	if (EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiBootServicesCode, FileNTHeaders->OptionalHeader.SizeOfImage / EFI_PAGE_SIZE, &ImagePagesPhysical)))
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	//
	// at this stage memory is identity mapped so the 
	// virtual address will be the same as the physical
	//
	*ImagePages = (PBYTE)*ImagePagesPhysical;

	return BL_STATUS_OK;
}

BL_STATUS
BLAPI
BlLdrLoadPEImage64(
	_In_ CHAR8* ImagePath,
	_Inout_ PBL_LDR_LOADED_IMAGE_INFO LoadedImageInfo
)
{
	if (ImagePath == NULL || LoadedImageInfo == NULL)
	{
		return BL_STATUS_INVALID_PARAMETER;
	}

	BL_LDR_FILE_IMAGE FileImage;
	//EfiZeroMemory(&FileImage, sizeof(BL_LDR_FILE_IMAGE));

	BL_STATUS Status = BlLdrLoadPEImageFile(ImagePath, &FileImage);
	if (BL_ERROR(Status))
	{
		return Status;
	}

	if (PeIsValidImage(FileImage.File) == FALSE)
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	PBYTE Image = NULL;
	EFI_PHYSICAL_ADDRESS ImagePhysical = NULL;
	if (EFI_ERROR(BlLdrAllocatePEImagePages(&FileImage, &Image, &ImagePhysical)))
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	return BL_STATUS_OK;
}