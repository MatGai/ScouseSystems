#include <ss/runtime/string.h>

void*
memcpy(
    void* dst,
    const void* src,
    unsigned long long sz
)
{
    char* destination  = (char*)dst;
    const char* source = (const char*)src;
    unsigned long long length = sz / sizeof( long );

    if( sz == 0 || dst == src )
    {
        return dst;
    }

    if( ( destination < source && destination + sz > source ) || 
        ( source < destination && source + sz > destination ) )
    {
        return NULL;
    }

    // copy per byte until destination is aligned to long
    while (
        sz > 0 && ( (unsigned long long)destination & ( sizeof( long ) - 1 ) ) 
        )
    {
        *destination++ = *source++;
        sz--;
    }


    // copy per 4 bytes (long) 
    long* dword = (long*)destination;
    const long* sword = (const long*)source;

    for( unsigned long long i = 0; i < sz / sizeof( long ); i++ )
    {
        dword[i] = sword[i];
    }
    
    unsigned long v = sz % sizeof( long );

    if (v != 0)
    {
        // copy remaining bytes
        destination = (char*)&dword[sz / sizeof(long)];
        source = (const char*)&sword[sz / sizeof(long)];

        for (unsigned long long i = 0; i < v; i++)
        {
            destination[i] = source[i];
        }
    }

    return dst;
}

void*
memset(
    void* dst,
    int v,
    unsigned long long sz
)
{
    if( sz == 0 )
    {
        return dst;
    }

    unsigned char* destination = dst;
    do 
    {
        *destination++ = (unsigned char)v;
    } while ( --sz != 0 );

    return dst;
}

int
memcmp(
    const void* dst,
    const void* src,
    unsigned long long sz
)
{
    if (sz == 0)
    {
        return 0;
    }

    unsigned char* p1 = dst;
    unsigned char* p2 = src;

    for( ; sz != 0; --sz )
    {
        if( *p1++ != *p2++ )
        {
            return ( *--p1 - *--p2 );
        }
    }
}

char*
strchr(
    const char* src,
    int c
)
{
    for( ;; ++src )
    {
        if( *src == (char)c )
        {
            return (char*)src;
        }
        if( !( *src ) )
        {
            return (char*)NULL;
        }
    }
}

int
strcmp(
    const char* s0,
    const char* s1
)
{
    while( *s0 == *s1++ )
    {
        if( *s0++ == 0 )
        {
            return 0;
        }
    }
    return (*(unsigned char*)s0 - *(unsigned char*)--s1);
}

char*
strlcpy(
    char* dst,
    const char* src,
    unsigned long long sz
)
{
    const char* src0 = src;
    unsigned long long left = sz;

    if( left != 0 )
    {
        while( --left != 0 )
        {
            if( (*dst++ = *src++) == '\0' )
            {
                break;
            }
        }
    }

    if (left == 0)
    {
        if( sz != 0 )
        {
            *dst = '\0';
        }
        while( *src++ );
    }

    return (src - src0 - 1);
}

unsigned long long
strlen(
    const char* str
)
{
    const char* str0;
    for (str0 = str; *str0; ++str0);
    return (str0 - str);
}

char*
strstr(
    const char* s0,
    const char* s1
)
{

}