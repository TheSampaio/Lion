#pragma once

#include <string>

#include <spdlog/spdlog.h>

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
