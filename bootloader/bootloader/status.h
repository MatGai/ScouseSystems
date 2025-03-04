#ifndef STATUS_H
#define STATUS_H

#include "stdint.h"

typedef LONG BL_STATUS;

#define BL_STATUS_WARNING_BASE 0xA0000000
#define BL_STATUS_ERROR_BASE 0xC0000000

#define BL_STATUS_OK ( LONG )0
#define BL_STATUS_GENERIC_ERROR ( LONG )BL_STATUS_ERROR_BASE

#define BL_SUCCESS( Status ) ( Status == BL_STATUS_OK )
#define BL_WARNING( Status ) ( ((Status) & 0xF0000000) == BL_STATUS_WARNING_BASE )
#define BL_ERROR( Status ) ( ((Status) & 0xF0000000) == BL_STATUS_ERROR_BASE )

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#ifdef _DEBUG

#undef DEBUG_ERROR
#undef DEBUG_INFO

#define DEBUG_INFO(Message, ...)    \
  do {  \
        CHAR16 WideFunction[256];   \
        AsciiToUnicode(__FUNCTION__, WideFunction, sizeof(WideFunction)/sizeof(CHAR16));    \
        CHAR16 file[256];   \
        AsciiToUnicode(__FILENAME__, file, sizeof(file) / sizeof(CHAR16)); \
        Print(L"[ INFO ] %s:%s:%d: " Message, file ,WideFunction, __LINE__, ##__VA_ARGS__);    \
  } while(0)

#define DEBUG_ERROR(Status, Message, ...) \
    do {    \
        CHAR16 WideFunction[256];   \
        AsciiToUnicode(__FUNCTION__, WideFunction, sizeof(WideFunction)/sizeof(CHAR16));   \
        CHAR16 file[256];   \
        AsciiToUnicode(__FILENAME__, file, sizeof(file) / sizeof(CHAR16));  \
        Print(L"[ ERROR ] [ %r ] %s:%s:%d: " Message, Status, file ,WideFunction, __LINE__, ##__VA_ARGS__); \
  } while(0)

#else
    #define INFO(...) ((VOID)0)
    #define ERROR(...) ((VOID)0)
#endif

#endif // ! STATUS_H


