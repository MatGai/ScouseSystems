#include "filesystem.h"

BOOLEAN 
BLAPI
BlInitFileSystem(
    VOID
)
{
    EFI_GUID LoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    FILE_SYSTEM_STATUS = gBS->HandleProtocol( gImageHandle, &LoadedImageProtocolGUID, (VOID**)&LoadedImage );

    if( EFI_ERROR( FILE_SYSTEM_STATUS ) )
    {
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Failed to get loaded image protocol");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
BLAPI
BlGetRootDirectory(
    _Out_opt_ EFI_FILE_PROTOCOL** Directory
)
{
    if (!LoadedImage)
    {
        DEBUG_ERROR(BlGetLastFileError(), L"Loaded image was null, maybe failed to get it ? ",  );
        return FALSE;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileProtocol;
    FILE_SYSTEM_STATUS = gBS->HandleProtocol(LoadedImage->DeviceHandle, &__FileSystemProtoclGUID__, (VOID**)&FileProtocol);

    if ( EFI_ERROR( FILE_SYSTEM_STATUS ) )
    {
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Could not get file protocol" );
        return FALSE;
    }

    EFI_FILE_PROTOCOL* Root;
    FILE_SYSTEM_STATUS = FileProtocol->OpenVolume(FileProtocol, &Root );

    if( EFI_ERROR( FILE_SYSTEM_STATUS )  )
    {
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Could not open the root directory");
        return FALSE;
    }

    CurrentDirectory = Root;

    if (Directory)
    {
        *Directory = Root;
    }

    return TRUE;
}

BOOLEAN
BLAPI
BlGetRootDirectoryByIndex(
    _In_ FILE_SYSTEM Index,
    _Out_opt_ EFI_FILE_PROTOCOL** Directory
)
{
    EFI_HANDLE* FileSystemHandles;
    ULONG64 HandleCount = 0;

    // Get all handles to all file systems on this system
    FILE_SYSTEM_STATUS = gBS->LocateHandleBuffer(ByProtocol, &__FileSystemProtoclGUID__, NULL, &HandleCount, &FileSystemHandles);

    if( EFI_ERROR( FILE_SYSTEM_STATUS ) || !HandleCount )
    {
        return FALSE;
    }

    // well this just shouldn't happen!
    if( (ULONG64)Index > HandleCount )
    {
        FreePool( FileSystemHandles );
        return FALSE;
    }

    // open the file system by handle index
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileProtocol;
    FILE_SYSTEM_STATUS = gBS->OpenProtocol( FileSystemHandles[ Index ], &__FileSystemProtoclGUID__, (VOID**)&FileProtocol, gImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
    
    if( EFI_ERROR( FILE_SYSTEM_STATUS ) || !FileProtocol )
    {
        FreePool( FileSystemHandles );
        return FALSE;
    }

    // now actually get the directory!
    EFI_FILE_PROTOCOL* Root;
    FILE_SYSTEM_STATUS = FileProtocol->OpenVolume(FileProtocol, &Root);

    if (EFI_ERROR(FILE_SYSTEM_STATUS))
    {
        return FALSE;
    }

    CurrentFileSystemHandle = FileSystemHandles[ Index ];
    CurrentDirectory = Root;

    if ( Directory )
    {
        *Directory = Root;
    }

    return TRUE;
}

BOOLEAN
BLAPI
BlOpenSubDirectory(
    _In_  EFI_FILE_PROTOCOL* BaseDirectory,
    _In_  CHAR16* Path,
    _Out_ EFI_FILE_PROTOCOL** OutDirectory
)
{
    if (!BaseDirectory || !Path || !OutDirectory) 
    {
        FILE_SYSTEM_STATUS = EFI_INVALID_PARAMETER;
        return FALSE;
    }

    EFI_FILE_PROTOCOL* NewDirectory = NULL;

    // passing 0 as attributes returns if it is directory or not
    FILE_SYSTEM_STATUS = BaseDirectory->Open(
        BaseDirectory,
        &NewDirectory,
        Path,
        EFI_FILE_MODE_READ,
        0
    );

    // this is not a directory!
    if (FILE_SYSTEM_STATUS != EFI_FILE_DIRECTORY)
    {
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Passed in path '%s' is not a directory to open!!!!", Path );
        FILE_SYSTEM_STATUS = EFI_INVALID_PARAMETER;
        return FALSE;
    }

    //pretty confident it is a directory now
    FILE_SYSTEM_STATUS = BaseDirectory->Open(
        BaseDirectory,
        &NewDirectory,
        Path,
        EFI_FILE_MODE_READ,
        EFI_FILE_DIRECTORY
    );
    
    if( EFI_ERROR( FILE_SYSTEM_STATUS ) )
    {
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Failed to open '%s' as directory", Path);
        return FALSE;
    }

    *OutDirectory = NewDirectory;
    return FALSE;
}

BOOLEAN
BLAPI
BlFindFile(
    _In_ LPCSTR File,
    _Out_ EFI_FILE_PROTOCOL** OutFile
)
{
    if (File == NULL)
    {
        return FALSE;
    }

    // Make sure we have a CurrentDirectory
    if (!CurrentDirectory) 
    {
        // default to fs0: root
        BlGetRootDirectory( NULL );

        // if this happens then...well...
        if (!CurrentDirectory)
        {
            return FALSE;
        }
    }

    EFI_FILE_PROTOCOL* OpenedFile = NULL;
    FILE_SYSTEM_STATUS = CurrentDirectory->Open(
        CurrentDirectory,
        &OpenedFile,
        File,
        EFI_FILE_MODE_READ,
        0
    );

    if (EFI_ERROR(FILE_SYSTEM_STATUS))
    {
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Failed to open file '%s'\n", File );
        return FALSE;
    }

    *OutFile = OpenedFile;
    return TRUE;
}

BOOLEAN
BLAPI
BlListDirectoryRecursive(
    _In_ EFI_FILE_PROTOCOL* Directory,
    _In_ ULONG64 Depth
)
{
    if (Directory == NULL) 
    {
        return FALSE;
    }

    // reset position so we read from the start
    FILE_SYSTEM_STATUS = Directory->SetPosition(Directory, 0);
    if (EFI_ERROR(FILE_SYSTEM_STATUS)) 
    {
        DEBUG_ERROR(L"Failed to SetPosition on directory\n");
        return FALSE;
    }

    // allocate a buffer for EFI_FILE_INFO. Sketchy as won't work for longer names.
    ULONG64 BufferSize = 0x128;
    EFI_FILE_INFO* FileInfo = AllocateZeroPool(BufferSize);
    if (!FileInfo) 
    {
        //probably out of resources?
        FILE_SYSTEM_STATUS = EFI_OUT_OF_RESOURCES;
        return FALSE;
    }

    // Keep reading entries until we reach the end of the directory
    while (TRUE) 
    {
        ULONG64 Size = BufferSize;
        FILE_SYSTEM_STATUS = Directory->Read(Directory, &Size, FileInfo);
        if (EFI_ERROR(FILE_SYSTEM_STATUS))
        {
            DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Failed to read directory 's'\n", FileInfo->FileName);
            FreePool(FileInfo);
            return FALSE;
        }

        if (Size == 0) 
        {
            //no more to read
            break;
        }

        // skip the '.' and '..' at the start of directories to avoid infinite recursion
        if ((StrCmp(FileInfo->FileName, L".") == 0) ||
            (StrCmp(FileInfo->FileName, L"..") == 0)) 
        {
            continue;
        }

        // just add indentation
        for (ULONG64 i = 0; i < Depth; i++) 
        {
            Print(L"  ");
        }

        //check if its directory and list its contents
        if (FileInfo->Attribute & EFI_FILE_DIRECTORY) 
        {
            Print(L"<DIR> %s\n", FileInfo->FileName);

            // try to open the subdirectory
            EFI_FILE_PROTOCOL* SubDirectory = NULL;
            FILE_SYSTEM_STATUS = Directory->Open(
                Directory,
                &SubDirectory,
                FileInfo->FileName,
                EFI_FILE_MODE_READ,
                0
            );

            if (EFI_ERROR(FILE_SYSTEM_STATUS) && !SubDirectory)
            {
                DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Cannot open subdirectory %s\n", FileInfo->FileName);
                FreePool(FileInfo);
                return FALSE;
            }

            // recursive call
            BlListDirectoryRecursive(SubDirectory, Depth + 1);
            // make sure to close
            SubDirectory->Close(SubDirectory);
        }
        else 
        {
            //if its a file then display contents
            Print(L"%-6d  %s\n", FileInfo->FileSize, FileInfo->FileName);
        }
    }

    FreePool(FileInfo);
    return TRUE;
}


BOOLEAN
BLAPI
BlListAllFiles(
    VOID
)
{
    if (!CurrentDirectory) 
    {
        //try get current directory
        BlGetRootDirectory(NULL);

        //well that is just nice
        if (!CurrentDirectory)
        {
            return FALSE;
        }
    }

    Print(L"\nDirectory listing -> \n");
    return BlListDirectoryRecursive(CurrentDirectory, 0);
}

EFI_STATUS 
BLAPI
BlGetLastFileError(
    VOID
)
{
    return FILE_SYSTEM_STATUS;
}
 
BOOLEAN
BLAPI
BlSetWorkingDirectory(
    _In_ LPCSTR Directory
)
{
    if (Directory == NULL || Directory[0] == '\0')
    {
        FILE_SYSTEM_STATUS = EFI_INVALID_PARAMETER;
        return FALSE;
    }

    // if no set directory set it!
    if ( CurrentDirectory == NULL ) 
    {
        if( !BlGetRootDirectory( NULL ) )
        {
            if (EFI_ERROR(FILE_SYSTEM_STATUS))
            {
                DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Failed to get root directory of fs0\n");
                return FALSE;
            }
        }
    }

    if ( CurrentDirectory ) 
    {
        EFI_FILE_PROTOCOL* NewDirectory = NULL;

        if ( BlOpenSubDirectory(CurrentDirectory, Directory, &NewDirectory) && NewDirectory )
        {
            // we got new direectory!
            CurrentDirectory = NewDirectory;
            return TRUE;
        }
 
        DEBUG_ERROR(FILE_SYSTEM_STATUS, L"Failed to get new directory '%s'\n", Directory );
    }

    return FALSE;
}

BOOLEAN
BlGetFileName(
    _In_ EFI_FILE_PROTOCOL* FileProtocol,
    _Out_ CHAR16** Out 
)
{
    if (FileProtocol == NULL) 
    {
        FILE_SYSTEM_STATUS = EFI_INVALID_PARAMETER;
        return FALSE;
    }

    ULONG64        InfoSize = 0x128;
    EFI_FILE_INFO* FileInfo = AllocateZeroPool(InfoSize);
    if ( !FileInfo ) 
    {
        FILE_SYSTEM_STATUS = EFI_OUT_OF_RESOURCES;
        return FALSE;
    }

    FILE_SYSTEM_STATUS = FileProtocol->GetInfo(
        FileProtocol,
        &gEfiFileInfoGuid,
        &InfoSize,
        FileInfo
    );

    if (EFI_ERROR(FILE_SYSTEM_STATUS))
    {
        FreePool(FileInfo);
        return FALSE;
    }

    strfmt(Out, L"%s", FileInfo->FileName);

    FreePool(FileInfo);
    return TRUE;
}

BOOLEAN
BlGetFileInfo(
    _In_ EFI_FILE_HANDLE FileHandle,
    _Inout_ EFI_FILE_INFO** FileInfo
)
{
    if (FileHandle == NULL || FileInfo == NULL)
    {
        FILE_SYSTEM_STATUS = EFI_INVALID_PARAMETER;

        return FALSE;
    }

    //
    // allocate for sizeof( EFI_FILE_INFO ) + padding for file name size
    //
    UINTN FileInfoInfoAllocSize = sizeof(EFI_FILE_INFO) + 1024;
    EFI_STATUS Result = gBS->AllocatePool(EfiBootServicesData, FileInfoInfoAllocSize, FileInfo);
    if (EFI_ERROR(Result))
    {
        FILE_SYSTEM_STATUS = Result;

        return FALSE;
    }

    Result = FileHandle->GetInfo(FileHandle, &gEfiFileInfoGuid, &FileInfoInfoAllocSize, *FileInfo);
    if (EFI_ERROR(Result))
    {
        //
        // if somehow file name size was > 1024 re allocate and try again
        //
        if (Result == EFI_BUFFER_TOO_SMALL)
        {
            gBS->FreePool(*FileInfo);
            Result = gBS->AllocatePool(EfiBootServicesData, FileInfoInfoAllocSize, FileInfo);
            if (EFI_ERROR(Result))
            {
                FILE_SYSTEM_STATUS = Result;

                return FALSE;
            }

            Result = FileHandle->GetInfo(FileHandle, &gEfiFileInfoGuid, &FileInfoInfoAllocSize, *FileInfo);

            if (EFI_ERROR(Result))
            {
                FILE_SYSTEM_STATUS = Result;

                gBS->FreePool(*FileInfo);

                return FALSE;
            }
        }
        else
        {
            FILE_SYSTEM_STATUS = Result;

            return FALSE;
        }
    }

    return TRUE;
}