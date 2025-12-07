
typedef unsigned __int64 (*__WriteConsole)( unsigned short* );

unsigned int 
ConsoleWrite(
	unsigned short* str
)
{
	return &str;
}

unsigned __int64 
putc(
	__WriteConsole WriteConsole,
	unsigned short character

)
{
	unsigned short tmp[2] = { character, '\0' };

	if (character == L'\n')
	{
		sputc( WriteConsole, L'\r' );
	}

	if (WriteConsole)
	{
		return WriteConsole( tmp );
	}

	return 0;
}

unsigned __int64
printf(
	unsigned __int16* Format,
	...
)
{
	va_list args;

}


void 
print(

)
{
	sputc(ConsoleWrite, L"A");
}