#include "Engine.h"
#include "WindowBackend.h"

#include <Lion/Platform/GlfwWindow.h>

namespace Lion
{
	Scope<WindowBackend> WindowBackend::Create()
	{
		// GLFW is currently the only windowing backend; it serves both OpenGL and Vulkan.
		return MakeScope<GlfwWindow>();
	}
}
