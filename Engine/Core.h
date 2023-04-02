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
#ifdef GLF_WIN
	#ifdef GLF_DLL
		#define GLF3D_API __declspec(dllexport)
	#else
		#define GLF3D_API __declspec(dllimport)
	#endif
#else
	#ifdef GLF_DLL
		#define GLF3D_API __attribute__((visibility("default")))
	#else
		#define GLF3D_API
	#endif
#endif

#endif // !GLF3D_CORE_H
