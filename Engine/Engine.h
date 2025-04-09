#pragma once

// DLL exporter
#ifdef LN_PLATFORM_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

	#ifdef LN_DLL
		#define LION_API __declspec(dllexport)
	#else
		#define LION_API __declspec(dllimport)
	#endif
#else
	#error Lion engine does not support Unix platforms!
#endif

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <stb_image/stb_image.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

#include "Source/Kind/Primitive.h"
