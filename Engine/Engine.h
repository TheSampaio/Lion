#pragma once

// Standard
#include <algorithm>
#include <assert.h>
#include <array>
#include <chrono>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

// Windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <windowsx.h>
#endif // !WIN32_LEAN_AND_MEAN

// Additional
#include <dxgi.h>
#include <d3d11.h>
#include <dxgiformat.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <timeapi.h>

// Windows Image Component (WIC)
#ifndef WIN32_WIC
#define WIN32_WIC
	#pragma warning(push)
	#pragma warning(disable : 4005)
		#include <stdint.h>
		#include <wincodec.h>
	#pragma warning(pop)
#endif // !WIN32_WIC

// Types
#include "Source/Kind/Header/Basic.h"

// DLL exporter
#ifdef LN_PLATFORM_WIN
	#ifdef LN_DLL
		#define LION_API __declspec(dllexport)
	#else
		#define LION_API __declspec(dllimport)
	#endif
#else
	#error Lion engine does not support Unix platforms!
#endif
