typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef unsigned __int64(__cdecl* OutputString)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This,
    unsigned short* String
    );

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_MODE {
    __int32 MaxMode;
    __int32 Mode;
    __int32 Attribute;
    __int32 CursorColumn;
    __int32 CursorRow;
    unsigned char CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    OutputString Print;
    void* TestString;

    void* QueryMode;
    void* SetMode;
    void* SetAttribute;
    void* ClearScreen;
    void* SetCursorPosition;
    void* EnableCursor;

    EFI_SIMPLE_TEXT_OUTPUT_MODE* Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;


int KernelMain(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut
)
{
    ConOut->Print(ConOut, L"Hello from kernel\r\n");
    return 1;
} 