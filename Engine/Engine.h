#pragma once

// Standard
#include <array>
#include <chrono>
#include <iostream>
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
#include <timeapi.h>

// Types
#include "Source/Math/Header/Types.h"

// DLL exporter
#ifdef WL_PLATFORM_WIN
	#ifdef WL_DLL
		#define OWL_API __declspec(dllexport)
	#else
		#define OWL_API __declspec(dllimport)
	#endif
#else
	#error Owl engine does not support Unix platforms!
#endif
