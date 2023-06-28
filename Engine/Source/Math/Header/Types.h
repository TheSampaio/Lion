#pragma once

// === Typedefs ======

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

// === Templates ======

// Releases a Microsoft's COM
template<typename TReturnType>
void ReleaseCOM(TReturnType Component)
{
	if (Component)
	{
		Component->Release();
		Component = nullptr;
	}
}

// === Defines ======

// Gets a unique identification code of a Microsoft's COM class
#define UUID(x) __uuidof(x)
