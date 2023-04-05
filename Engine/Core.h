// === PRECOMPILED HEADER === //
#ifndef GLF3D_CORE_H
#define GLF3D_CORE_H

// Standard
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Additional
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

// === DLL EXPORTER === //
#ifdef _WIN
	#ifdef _DLL
		#define OWL_API __declspec(dllexport)
	#else
		#define OWL_API __declspec(dllimport)
	#endif
#else
	#ifdef _DLL
		#define OWL_API __attribute__((visibility("default")))
	#else
		#define OWL_API
	#endif
#endif

#endif
