#pragma once

#pragma warning(push)
#pragma warning(disable: LN_DISABLE_WARNINGS)

	#include <box2d/box2d.h>

	#include <glad/glad.h>
	#include <glfw/glfw3.h>

	#include <imgui/imgui.h>
	#include <imgui/backends/imgui_impl_glfw.h>
	#include <imgui/backends/imgui_impl_opengl3.h>

	#include <spdlog/spdlog.h>
	#include <stb_image/stb_image.h>

	#include <glm/glm.hpp>
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/constants.hpp>
	#include <glm/gtc/matrix_transform.hpp>

	#define GLM_ENABLE_EXPERIMENTAL
	#include <glm/gtx/norm.hpp>                     // Optional for fast normalization
	#include <glm/gtx/euler_angles.hpp>

#pragma warning(pop)
