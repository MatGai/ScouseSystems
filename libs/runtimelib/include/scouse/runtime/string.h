#ifndef RT_STRING_H
#define RT_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static_assert( sizeof(long) == 4, "Long is not 4 bytes");

//
//    MEMORY  
//

void* 
memcpy(
    void* dst,
    const void* src,
    unsigned long long sz
);

void*
memset(
    void* dst,
    int v,
    unsigned long long sz
);

int
memcmp(
    const void* dst,
    const void* src,
    unsigned long long sz
);

//
//   STRING  
//

char*
strchr(
    const char* src,
    int c
);

int
strcmp(
    const char* s0,
    const char* s1
);

char*
strlcpy(
    char* dst,
    const char* src,
    unsigned long long sz
);

unsigned long long
strlen(
    const char* str
);

char*
strstr(
    const char* s0,
    const char* s1
);

#ifdef __cplusplus
}
#endif

#endif // !RT_STRING_H