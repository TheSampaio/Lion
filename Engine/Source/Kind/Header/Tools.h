#pragma once

// Gets a unique identification code of a Microsoft's COM class
#define UUID(x) __uuidof(x)

// Releases a Microsoft's COM
template<typename TReturnType>
void ReleaseCOM(TReturnType Component)
{
	if (Component)
	{
		Component->Release();
		Component = nullptr;
	}
};
