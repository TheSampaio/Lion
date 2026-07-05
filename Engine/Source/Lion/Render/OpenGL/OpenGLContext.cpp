#include "Engine.h"
#include "OpenGLContext.h"

#include <Lion/Core/Log.h>

namespace Lion
{
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: mWindowHandle(windowHandle)
	{
	}

	bool OpenGLContext::Init()
	{
		if (!mWindowHandle)
		{
			Log::Console(LogLevel::Error, "[OpenGLContext] No valid window handle. Initialization aborted.");
			return false;
		}

		glfwMakeContextCurrent(mWindowHandle);

		if (!gladLoadGL())
		{
			Log::Console(LogLevel::Error, "[OpenGLContext] GLAD initialization failed.");
			return false;
		}

		return true;
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(mWindowHandle);
	}

	void OpenGLContext::SetVerticalSync(bool enable)
	{
		glfwSwapInterval(enable ? 1 : 0);
	}

	std::string OpenGLContext::GetDeviceName() const
	{
		return reinterpret_cast<const char8*>(glGetString(GL_RENDERER));
	}

	std::string OpenGLContext::GetApiVersion() const
	{
		return reinterpret_cast<const char8*>(glGetString(GL_VERSION));
	}
}
