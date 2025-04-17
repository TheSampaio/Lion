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

#pragma warning(push, 0)
#pragma warning(disable : 900 1694 2567 6294 26495 26498 26800)

	// Standard
	#include <array>
	#include <cstdint>        // For fixed size integer types
	#include <cstddef>        // For std::size_t
	#include <format>
	#include <fstream>
	#include <functional>
	#include <memory>
	#include <sstream>
	#include <string>
	#include <unordered_map>
	#include <vector>

	// External
	#include <glad/glad.h>
	#include <glfw/glfw3.h>
	#include <spdlog/spdlog.h>
	#include <stb_image/stb_image.h>

	#include <glm/glm.hpp>
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/matrix_transform.hpp>

	#define GLM_ENABLE_EXPERIMENTAL
	#include <glm/gtx/euler_angles.hpp>

#pragma warning(pop)

#include "Source/Kind/Allocators.h"
#include "Source/Kind/Macros.h"
#include "Source/Kind/Primitives.h"
