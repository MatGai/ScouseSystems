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

#define TLBTEST_PAGE_SIZE      4096
#define TLBTEST_NUM_PAGES      1024
#define TLBTEST_ITERATIONS     1000000ULL

// Page aligned buffer that we fill with jmps/ret.
__declspec(align(TLBTEST_PAGE_SIZE))
static unsigned char g_TlbJumpPages[TLBTEST_NUM_PAGES * TLBTEST_PAGE_SIZE];

uint16_t HelloBuffer[] = L"Hello from kernel buffer!\n";

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
    ConOut->Print(ConOut, HelloBuffer);

    //RunTlbJumpTest(ConOut);

    return 1;
}

#include <intrin.h>

#pragma intrinsic(__rdtsc)

unsigned __int64 __readtscserial(
    void
)
{
    _mm_lfence();
    unsigned __int64 t = __rdtsc();
    _mm_lfence();
    return t;
}

static __forceinline TLBTEST_STUB
TlbTestGetStubInPage(
    unsigned int pageIndex
)
{
    //unsigned char* base = g_TlbJumpPages + ((unsigned __int64)pageIndex * TLBTEST_PAGE_SIZE);
    //return (TLBTEST_STUB)base;
    return 0;
}

static void
ConOutPrintDecimal(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut,
    unsigned __int64 value
)
{
    unsigned short buf[32];  // enough for 20 digits + null
    int pos = 31;

    buf[pos] = 0; // null-terminate

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
RunTlbJumpTest(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut)
{
    unsigned int i;
    unsigned __int64 start, end;
    unsigned __int64 deltaSame, deltaCross;

    ConOut->Print(ConOut, L"\r\n[TLB] Setting up jump pages...\r\n");

    //
    // Fill each page with a single instruction: ret
    //
    for (i = 0; i < TLBTEST_NUM_PAGES; ++i)
    {
        unsigned char* p = (unsigned char*)TlbTestGetStubInPage(i);
        p[0] = 0xC3; // ret
    }

    ConOut->Print(ConOut, L"Hello2\r\n");

    //
    // prefetch pages for tlb
    //
    for (i = 0; i < TLBTEST_NUM_PAGES; ++i)
    {
        TLBTEST_STUB stub = TlbTestGetStubInPage(i);
        stub();
    }

    //
    // repeatedly call a stub in a single page.
    //
    ConOut->Print(ConOut, L"[TLB] Same-page jump test...\r\n");

    TLBTEST_STUB samePageStub = TlbTestGetStubInPage(0);

    start = __readtscserial();
    for (unsigned __int64 iter = 0; iter < TLBTEST_ITERATIONS; ++iter)
    {
        samePageStub();
    }
    end = __readtscserial();

    deltaSame = end - start;

    ConOut->Print(ConOut, L"[TLB] Same page total cycles: ");
    ConOutPrintDecimal(ConOut, deltaSame);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Same page cycles per call: ");
    ConOutPrintDecimal(ConOut, deltaSame / TLBTEST_ITERATIONS);
    ConOut->Print(ConOut, L"\r\n");

    //
    // for each call, jump into a different page.
    //
    ConOut->Print(ConOut, L"[TLB] Cross-page jump test...\r\n");

    start = __readtscserial();
    unsigned int pageIndex = 0;

    for (unsigned __int64 iter = 0; iter < TLBTEST_ITERATIONS; ++iter)
    {
        TLBTEST_STUB stub = TlbTestGetStubInPage(pageIndex);
        stub();
        pageIndex = (pageIndex + 1) & (TLBTEST_NUM_PAGES - 1);
    }

    end = __readtscserial();

    deltaCross = end - start;

    ConOut->Print(ConOut, L"[TLB] Cross-page total cycles: ");
    ConOutPrintDecimal(ConOut, deltaCross);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Cross-page cycles per call: ");
    ConOutPrintDecimal(ConOut, deltaCross / TLBTEST_ITERATIONS);
    ConOut->Print(ConOut, L"\r\n");

    ConOut->Print(ConOut, L"[TLB] Done.\r\n\r\n");
}