#ifndef STATUS_H
#define STATUS_H

#include "stdint.h"

typedef LONG BL_STATUS;

#define BL_STATUS_INFO_BASE 0x70000000
#define BL_STATUS_WARNING_BASE 0xA0000000
#define BL_STATUS_ERROR_BASE 0xC0000000

#define BL_STATUS_DEFINE_INFO( Code ) ( BL_STATUS_INFO_BASE + Code )
#define BL_STATUS_DEFINE_ERROR( Code ) ( BL_STATUS_ERROR_BASE + Code )
#define BL_STATUS_DEFINE_WARNING( Code ) ( BL_STATUS_WARNING_BASE + Code )

#define BL_STATUS_OK ( LONG )0
#define BL_STATUS_GENERIC_ERROR BL_STATUS_DEFINE_ERROR( 0 )
#define BL_STATUS_INVALID_PARAMETER BL_STATUS_DEFINE_ERROR( 1 )
#define BL_STATUS_INVALID_PARAMTER_ONE BL_STATUS_DEFINE_ERROR( 2 )
#define BL_STATUS_INVALID_PARAMTER_TWO BL_STATUS_DEFINE_ERROR( 3 )
#define BL_STATUS_INVALID_PARAMTER_THREE BL_STATUS_DEFINE_ERROR( 4 )
#define BL_STATUS_INVALID_PATH BL_STATUS_DEFINE_ERROR( 10 )
#define BL_STATUS_EFI_ERROR BL_STATUS_DEFINE_ERROR( 11 )

#define BL_SUCCESS( Status ) ( Status == BL_STATUS_OK )
#define BL_WARNING( Status ) ( ((Status) & 0xF0000000) == BL_STATUS_WARNING_BASE )
#define BL_ERROR( Status ) ( ((Status) & 0xF0000000) == BL_STATUS_ERROR_BASE )



#ifdef _DEBUG

#undef DEBUG_ERROR
#undef DEBUG_INFO

#define __PRINT_DEBUG( Level, Message, ... )                                                   \
  do {                                                                                         \
        CHAR16 WideFunction[256];                                                              \
        AsciiToUnicode(__FUNCTION__, WideFunction, sizeof(WideFunction)/sizeof(CHAR16));       \
        Print(L"[ %s ] %s:%d: " Message, Level, WideFunction, __LINE__, ##__VA_ARGS__);  \
  } while(0)                                                                                   

#define DEBUG_INFO(Message, ...)                         \
  do {                                                   \
        __PRINT_DEBUG( L"Info", Message, ##__VA_ARGS__ );  \
  } while(0)

#define DEBUG_ERROR(Status, Message, ...)                \
  do {                                                   \
        __PRINT_DEBUG(  Status, Message, ##__VA_ARGS__ );  \
  } while(0)

 #define DEBUG_ASSERT( Expression, Return, Message, ... )       \
    do {                                                        \
        if ( !(Expression) )                                    \
        {                                                       \
            __PRINT_DEBUG( L"Assert", Message, ##__VA_ARGS__ ); \
            return Return;                                      \
        }                                                       \
    } while(0)
    

#else
    #define DEBUG_INFO(...) ((VOID)0)
    #define DEBUG_ERROR(...) ((VOID)0)
    #define DEBUG_ASSERT( Expression, Return, Message, ... ) \
        if ( !(Expression) )  \
        {   \
            return Return;  \
        }   \
#endif

#endif // ! STATUS_H
#endif // ! STATUS_H



