// === PRECOMPILED HEADER === //
#ifndef GLF3D_CORE_H
#define GLF3D_CORE_H

// Standard
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

// Additional
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

// === DLL EXPORTER === //
#ifdef WL_PLATFORM_WIN
	#ifdef WL_DLL
		#define OWL_API __declspec(dllexport)
	#else
		#define OWL_API __declspec(dllimport)
	#endif
#else
	#ifdef WL_DLL
		#define OWL_API __attribute__((visibility("default")))
	#else
		#define OWL_API
	#endif
#endif

#endif
