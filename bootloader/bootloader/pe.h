#pragma once

#include "stdint.h"

#include <IndustryStandard/PeImage.h>

typedef EFI_IMAGE_DOS_HEADER* PEFI_IMAGE_DOS_HEADER;

typedef EFI_IMAGE_NT_HEADERS64 EFI_IMAGE_NT_HEADERS;
typedef EFI_IMAGE_NT_HEADERS64* PEFI_IMAGE_NT_HEADERS;

#define EFI_IMAGE_FIRST_SECTION(ntheader) \
    ( \
      (EFI_IMAGE_SECTION_HEADER *) \
        ( \
          (UINT64) ntheader + \
          offsetof (EFI_IMAGE_NT_HEADERS, OptionalHeader) + \
          ((EFI_IMAGE_NT_HEADERS *) (ntheader))->FileHeader.SizeOfOptionalHeader \
        ) \
    )

#define EFI_IMAGE_NTHEADERS( image ) ( ( PEFI_IMAGE_NT_HEADERS )( ( ( PEFI_IMAGE_DOS_HEADER )image )->e_lfanew + ( PBYTE )image ) )

//
// EDK2 Have the wrong definitions for some of the import
// structures, make sure not to use them
//

typedef struct _IMAGE_THUNK_DATA
{
	union
	{
		UINT64 ForwarderString;
		UINT64 Function;
		UINT64 Ordinal;
		UINT64 AddressOfData;

	} u1;

} IMAGE_THUNK_DATA, * PIMAGE_THUNK_DATA;

typedef struct _IMAGE_IMPORT_DESCRIPTOR
{
	union
	{
		UINT32 Characteristics;			// 0 for terminating null import descriptor
		UINT32 OriginalFirstThunk;		// RVA to original unbound IAT (PIMAGE_THUNK_DATA)

	} u;

	UINT32 TimeDateStamp;
	UINT32 ForwarderChain;				// -1 if no forwarders
	UINT32 Name;
	UINT32 FirstThunk;					// RVA to IAT (if bound this IAT has actual addresses)

} IMAGE_IMPORT_DESCRIPTOR, * PIMAGE_IMPORT_DESCRIPTOR;

BOOLEAN
PeIsValidImage(
	_In_ PBYTE PE
);