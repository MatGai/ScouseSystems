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

typedef struct _BOOT_INFO {
    unsigned __int64 DirectMapBase;
    unsigned __int64 Pml4Physical;
} BOOT_INFO, * PBOOT_INFO;

#include <stdint.h>

#define TLBTEST_PAGE_SIZE      0x1000
#define TLBTEST_NUM_PAGES      8888
#define TLBTEST_ITERATIONS     0xFFFFFF


// Page aligned buffer that we fill with jmps/ret.
__declspec(align(TLBTEST_PAGE_SIZE))
static unsigned char g_TlbJumpPages[TLBTEST_NUM_PAGES * TLBTEST_PAGE_SIZE];

typedef void (*TLBTEST_STUB)(void);

static __forceinline TLBTEST_STUB
TlbTestGetStubInPage(
    unsigned int pageIndex
);

static void
ConOutPrintDecimal(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut,
    unsigned __int64 value
);

static void
RunTlbJumpTest(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut
);

int KernelMain(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut,
    PBOOT_INFO BootInfo
)
{
    ConOut->Print(ConOut, L"Hello from kernel buffer!\r\n");
    RunTlbJumpTest(ConOut);

    return 1;
}

#include <intrin.h>

#pragma intrinsic(__rdtsc)

void
__flushtlb(
    void
)
{
    unsigned __int64 cr3 = __readcr3();
    __writecr3(cr3);
}

unsigned __int64 
__readtscserial(
    void
)
{
    unsigned __int64 t = __rdtsc();
    return t;
}

static __forceinline TLBTEST_STUB
TlbTestGetStubInPage(
    unsigned int PageIndex
)
{
    unsigned char* base = g_TlbJumpPages + ((unsigned __int64)PageIndex * TLBTEST_PAGE_SIZE);
    return (TLBTEST_STUB)base;
}

static void
ConOutPrintDecimal(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut,
    unsigned __int64 value
)
{
    unsigned short buf[32];  // enough for 20 digits + null
    int pos = 31;

    buf[pos] = L'\0'; // null-terminate

    if (value == 0)
    {
        buf[--pos] = L'0';
    }
    else
    {
        while (value > 0 && pos > 0)
        {
            unsigned __int64 q = value / 10;
            unsigned __int64 r = value - q * 10; // value % 10 
            value = q;
            buf[--pos] = (unsigned short)(L'0' + (unsigned short)r);
        }
    }

    ConOut->Print(ConOut, &buf[pos]);
}

static void
RunTlbJumpTest(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut
)
{
    unsigned int Index;
    unsigned __int64 Start, End;
    unsigned __int64 DeltaSamePage, DeltaCrossPage;

    ConOut->Print(ConOut, L"\r\n[TLB] Setting up jump pages...\r\n");

    //
    // Fill each page with a single instruction: ret
    //
    for (Index = 0; Index < TLBTEST_NUM_PAGES; ++Index)
    {
        unsigned char* p = (unsigned char*)TlbTestGetStubInPage(Index);
        p[0] = 0xC3; // ret
    }

    //
    // prefetch pages for tlb
    //
    for (Index = 0; Index < TLBTEST_NUM_PAGES; ++Index)
    {
        TLBTEST_STUB Stub = TlbTestGetStubInPage(Index);
        Stub();
    }

    //
    // repeatedly call a stub in a single page.
    //
    ConOut->Print(ConOut, L"[TLB] Same-page jump test...\r\n");

    __flushtlb();

    Start = __rdtsc();
    for ( Index = 0 ; Index < TLBTEST_ITERATIONS; ++Index )
    {
        TLBTEST_STUB SamePageStub = TlbTestGetStubInPage(0);
        SamePageStub();

        if (Index % TLBTEST_NUM_PAGES)
        {
            DeltaSamePage = (End - Start) % TLBTEST_NUM_PAGES;
        }
    }
    End = __rdtsc();

    DeltaSamePage = End - Start;

    ConOut->Print(ConOut, L"[TLB] Same page total cycles: ");
    ConOutPrintDecimal(ConOut, DeltaSamePage);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Same page cycles per call: ");
    ConOutPrintDecimal(ConOut, DeltaSamePage / TLBTEST_ITERATIONS);
    ConOut->Print(ConOut, L"\r\n");

    __flushtlb();

    Start = __rdtsc();
    for (Index = 0; Index < TLBTEST_ITERATIONS; ++Index)
    {
        TLBTEST_STUB SamePageStub = TlbTestGetStubInPage(0);
        SamePageStub();

		if (Index % TLBTEST_NUM_PAGES)
        {
			DeltaSamePage = (End - Start) % TLBTEST_NUM_PAGES;
        }
    }
    End = __rdtsc();

    DeltaSamePage = End - Start;

    ConOut->Print(ConOut, L"[TLB] Same page total cycles: ");
    ConOutPrintDecimal(ConOut, DeltaSamePage);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Same page cycles per call: ");
    ConOutPrintDecimal(ConOut, DeltaSamePage / TLBTEST_ITERATIONS);
    ConOut->Print(ConOut, L"\r\n");

    //
    // for each call, jump into a different page.
    //
    ConOut->Print(ConOut, L"[TLB] Cross-page jump test...\r\n");

    __flushtlb();

    Start = __rdtsc();
    unsigned int PageIndex = 0;

    for (Index = 0; Index < TLBTEST_ITERATIONS; ++Index)
    {
        TLBTEST_STUB Stub = TlbTestGetStubInPage(PageIndex);
        Stub();

		PageIndex = (PageIndex + 1) % TLBTEST_NUM_PAGES;
    }

    End = __rdtsc();

    DeltaCrossPage = End - Start;

    ConOut->Print(ConOut, L"[TLB] Cross-page total cycles: ");
    ConOutPrintDecimal(ConOut, DeltaCrossPage);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Cross-page cycles per call: ");
    ConOutPrintDecimal(ConOut, DeltaCrossPage / TLBTEST_ITERATIONS);
    ConOut->Print(ConOut, L"\r\n");

    __flushtlb();

    Start = __rdtsc();
    PageIndex = 0;

    for (Index = 0; Index < TLBTEST_ITERATIONS; ++Index)
    {
        TLBTEST_STUB Stub = TlbTestGetStubInPage(PageIndex);
        Stub();

        PageIndex = (PageIndex + 1) % TLBTEST_NUM_PAGES;
    }

    End = __rdtsc();

    DeltaCrossPage = End - Start;

    ConOut->Print(ConOut, L"[TLB] Cross-page total cycles: ");
    ConOutPrintDecimal(ConOut, DeltaCrossPage);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Cross-page cycles per call: ");
    ConOutPrintDecimal(ConOut, DeltaCrossPage / TLBTEST_ITERATIONS);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Done.\r\n\r\n");
}