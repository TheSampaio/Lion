#pragma once

#include <format>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

#include "Source/Kind/Primitive.h"

// DLL exporter
#ifdef LN_PLATFORM_WIN

#include <windows.h>

	#ifdef LN_DLL
		#define LION_API __declspec(dllexport)
	#else
		#define LION_API __declspec(dllimport)
	#endif
#else
	#error Lion engine does not support Unix platforms!
#endif
