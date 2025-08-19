#include "image.h"
#include "pe.h"
#include "filesystem.h"

BL_STATUS
BLAPI
BlLdrLoadPEImageFile(
	_In_ PCWSTR ImagePath,
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
		ImageFileHandle->Close(ImageFileHandle);
		return BL_STATUS_INVALID_PATH;
	}

	EFI_FILE_INFO* ImageFileInfo = NULL;
	if (BlGetFileInfo(ImageFileHandle, &ImageFileInfo) == FALSE)
	{
		ImageFileHandle->Close(ImageFileHandle);
		return BL_STATUS_GENERIC_ERROR;
	}

	UINTN FileImageSize = ImageFileInfo->FileSize;

	gBS->FreePool(ImageFileInfo);

	PBYTE FileImage = NULL;
	if (EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, FileImageSize, &FileImage)))
	{
		ImageFileHandle->Close(ImageFileHandle);
		return BL_STATUS_GENERIC_ERROR;
	}

	if (EFI_ERROR(ImageFileHandle->Read(ImageFileHandle, &FileImageSize, FileImage)))
	{
		ImageFileHandle->Close(ImageFileHandle);
		return BL_STATUS_GENERIC_ERROR;
	}

	ImageFileHandle->Close(ImageFileHandle);
	FileImageData->File = FileImage;
	FileImageData->FileSize = FileImageSize;

	return BL_STATUS_OK;
}

BL_STATUS
BLAPI
BlLdrAllocatePEImagePages(
	_In_ PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE* ImagePages,
	_Out_ EFI_PHYSICAL_ADDRESS* ImagePagesPhysical
)
{
	if (FileImage == NULL || ImagePagesPhysical == NULL || ImagePages == NULL)
	{
		return BL_STATUS_INVALID_PARAMETER;
	}

	PEFI_IMAGE_NT_HEADERS FileNtHeaders = EFI_IMAGE_NTHEADERS(FileImage->File);
	ULONG64 Pages = EFI_SIZE_TO_PAGES( FileNtHeaders->OptionalHeader.SizeOfImage );
	//*ImagePagesPhysical = FileNtHeaders->OptionalHeader.ImageBase;

    EFI_PHYSICAL_ADDRESS ImageBase = FileNtHeaders->OptionalHeader.ImageBase;

	//
	// try to allocate at prefered base
	//
	//gBS->AllocatePages(AllocateAnyPages/*AllocateAddress*/, EfiBootServicesCode, Pages, ImagePagesPhysical);
	EFI_STATUS PageStatus = gBS->AllocatePages(/*AllocateAnyPages*/AllocateAddress, EfiBootServicesCode, Pages, &ImageBase);

	if (EFI_ERROR(PageStatus))
	{
		if (PageStatus == EFI_NOT_FOUND)
		{

			//
			// oh well, we can just fix relocations
			//
			PageStatus = gBS->AllocatePages(AllocateAnyPages/*AllocateAddress*/, EfiBootServicesCode, Pages, &ImageBase);

			if( EFI_ERROR(PageStatus) )
			{
				//
				// we failed to allocate memory for the image
				//
				return BL_STATUS_GENERIC_ERROR;
            }
		}
		else
		{
			return BL_STATUS_GENERIC_ERROR;
		}
	}

	// not really needed...i hope...
	//ZeroMem(
	//	*ImagePagesPhysical,
	//	(Pages << EFI_PAGE_SHIFT)
	//);

	CopyMem(
		(PVOID)(UINTN)ImageBase,
		(PVOID)FileImage->File, 
		FileNtHeaders->OptionalHeader.SizeOfHeaders
	);

	//
	// at this stage memory is identity mapped so the 
	// virtual address will be the same as the physical
	//
    *ImagePagesPhysical = ImageBase;
	*ImagePages = (PBYTE)(UINTN)ImageBase;

	return BL_STATUS_OK;
}

BL_STATUS
BLAPI
BlLdrLoadPEImage64(
	_In_ PCWSTR ImagePath,
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
	if (BL_ERROR(BlLdrAllocatePEImagePages(&FileImage, &Image, &ImagePhysical)))
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	if (PeIsValidImage(Image) == FALSE)
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	if (BL_ERROR(BlLdrAlignFileImage(&FileImage, Image)))
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	if ( BL_ERROR( BlLdrImageRelocation(&FileImage, Image) ) )
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	EFI_IMAGE_DOS_HEADER* ImageDosHeader = (EFI_IMAGE_DOS_HEADER*)Image;
	EFI_IMAGE_NT_HEADERS* ImageNtHeaders = (EFI_IMAGE_NT_HEADERS*)((ULONG64)Image + ImageDosHeader->e_lfanew);

	LoadedImageInfo->Base = (ULONG64)Image;
	LoadedImageInfo->EntryPoint = (ULONG64)Image + (ULONG64)ImageNtHeaders->OptionalHeader.AddressOfEntryPoint;
	LoadedImageInfo->Size = (ULONG64)ImageNtHeaders->OptionalHeader.SizeOfImage;

	return BL_STATUS_OK;
}

BL_STATUS
BLAPI
BlLdrAlignFileImage(
	_In_    PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE Image
)
{
	if (!Image)
	{
		return BL_STATUS_INVALID_PARAMETER;
	}

	if (PeIsValidImage(Image) == FALSE)
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	EFI_IMAGE_DOS_HEADER* ImageDosHeader = (EFI_IMAGE_DOS_HEADER*)Image;
	EFI_IMAGE_NT_HEADERS* ImageNtHeaders = (EFI_IMAGE_NT_HEADERS*)(Image + ImageDosHeader->e_lfanew);

	//
	// use the IMAGE_FIRST_SECTION macro to get the pointer to the first section
	//
	EFI_IMAGE_SECTION_HEADER* FileSectionHeader = EFI_IMAGE_FIRST_SECTION(ImageNtHeaders);

	//
	// loop through each section and copy its raw data to the appropriate offset
	// in the new memory buffer based on the sections virtual address
	//
	for (UINT64 i = 0; i < ImageNtHeaders->FileHeader.NumberOfSections; i++)
	{
		EFI_IMAGE_SECTION_HEADER* CurrentSection = &FileSectionHeader[i];

		EFI_PHYSICAL_ADDRESS SectionAddress = (EFI_PHYSICAL_ADDRESS)Image + (EFI_PHYSICAL_ADDRESS)CurrentSection->VirtualAddress;
		UINT8* SectionSource				= FileImage->File + CurrentSection->PointerToRawData;

		if (CurrentSection->SizeOfRawData > 0)
		{
			CopyMem(
				(PVOID)(UINTN)SectionAddress,
				(PVOID)(UINTN)SectionSource,
				CurrentSection->SizeOfRawData
			);
		}
	}

	return BL_STATUS_OK;
}

BL_STATUS
BLAPI
BlLdrImageRelocation(
	_In_    PBL_LDR_FILE_IMAGE FileImage,
	_Inout_ PBYTE Image
)
{
	if (!Image || !FileImage)
	{
		return BL_STATUS_INVALID_PARAMETER;
	}

	if (PeIsValidImage(Image) == FALSE)
	{
		return BL_STATUS_GENERIC_ERROR;
	}

	EFI_IMAGE_DOS_HEADER* ImageDosHeader = (EFI_IMAGE_DOS_HEADER*)Image;
	EFI_IMAGE_NT_HEADERS* ImageNtHeaders = (EFI_IMAGE_NT_HEADERS*)(Image + ImageDosHeader->e_lfanew);

	if (((UINT64)Image != ImageNtHeaders->OptionalHeader.ImageBase) && (ImageNtHeaders->OptionalHeader.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC))
	{

		EFI_IMAGE_BASE_RELOCATION* ImageBaseRelocation = (EFI_IMAGE_BASE_RELOCATION*)(Image + (ULONG64)ImageNtHeaders->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		EFI_IMAGE_BASE_RELOCATION* ImageBaseRelocationEnd = (EFI_IMAGE_BASE_RELOCATION*)(Image + (ULONG64)ImageNtHeaders->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size + (ULONG64)ImageNtHeaders->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

		ULONG64 RelativeOffset;

		if ((UINT64)Image > ImageNtHeaders->OptionalHeader.ImageBase)
		{
			RelativeOffset = (UINT64)Image - ImageNtHeaders->OptionalHeader.ImageBase;
		}
		else
		{
			RelativeOffset = ImageNtHeaders->OptionalHeader.ImageBase - (UINT64)Image;
		}

		ULONG64 NumberOfRelocations;

		for (; (ImageBaseRelocation->SizeOfBlock) && (ImageBaseRelocation < ImageBaseRelocationEnd); )
		{
			EFI_PHYSICAL_ADDRESS Page = (UINT64)Image + (ULONG64)ImageBaseRelocation->VirtualAddress;
			UINT16* DataToFix = (UINT16*)((UINT8*)ImageBaseRelocation + EFI_IMAGE_SIZEOF_BASE_RELOCATION);
			NumberOfRelocations = (ImageBaseRelocation->SizeOfBlock - EFI_IMAGE_SIZEOF_BASE_RELOCATION) / sizeof(UINT16);

			for (UINT64 i = 0; i < NumberOfRelocations; i++)
			{
				UINT16 DataType = DataToFix[i] >> 12;
				switch (DataType)
				{
				case EFI_IMAGE_REL_BASED_ABSOLUTE:
				{
					break;
				}

				case EFI_IMAGE_REL_BASED_DIR64:
				{
					if ((UINT64)Image > ImageNtHeaders->OptionalHeader.ImageBase)
					{
						*((UINT64*)((UINT8*)Page + (DataToFix[i] & EFI_PAGE_MASK))) += RelativeOffset;
					}
					else
					{
						*((UINT64*)((UINT8*)Page + (DataToFix[i] & EFI_PAGE_MASK))) -= RelativeOffset;
					}
					break;
				}

				default:
				{
					return BL_STATUS_GENERIC_ERROR;
					break;
				}
				}

			}
			//
			// Get the next relocation chunk
			//
			ImageBaseRelocation = (EFI_IMAGE_BASE_RELOCATION*)((UINT8*)ImageBaseRelocation + ImageBaseRelocation->SizeOfBlock);
		}
	}

	return BL_STATUS_OK;
}