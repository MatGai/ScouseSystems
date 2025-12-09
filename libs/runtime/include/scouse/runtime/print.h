#ifndef RT_PRINT_H
#define RT_PRINT_H

#include <scouse/runtime/string.h>

typedef unsigned __int64 (*__WriteConsole)(unsigned short*);

unsigned __int64
putc(
	__WriteConsole WriteConsole,
	unsigned short Character
);

unsigned __int64
_vsnprintf(
	__WriteConsole WriteConsole,
	unsigned __int16* Format,
	va_list Args
);

unsigned __int64
printf(
	unsigned __int16* Format,
	...
);

#endif // !RT_PRINT_H
