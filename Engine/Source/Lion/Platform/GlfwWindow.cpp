#include "Engine.h"
#include "GlfwWindow.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Signal/EventInput.h>
#include <Lion/Signal/EventWindow.h>

namespace Lion
{
	GlfwWindow::~GlfwWindow()
	{
		if (mIcon)
		{
			stbi_image_free(mIcon->pixels);
			delete mIcon;
		}

		if (mWindow)
			glfwDestroyWindow(mWindow);

		glfwTerminate();
	}

	bool GlfwWindow::Initialize(WindowData* data)
	{
		mData = data;

		if (!glfwInit())
		{
			Log::Console(LogLevel::Fatal, "[GlfwWindow] GLFW initialization failed.");
			return false;
		}

		// Match the window's client API to the selected graphics backend: an OpenGL context for
		// OpenGL, or no context for Vulkan (which manages its own surface and device).
		glfwWindowHint(GLFW_CLIENT_API, RendererAPI::GetAPI() == GraphicsAPI::OpenGL ? GLFW_OPENGL_API : GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, false);
		glfwWindowHint(GLFW_RESIZABLE, mData->resizable ? GLFW_TRUE : GLFW_FALSE);

		mWindow = glfwCreateWindow(
			static_cast<int32>(mData->width),
			static_cast<int32>(mData->height),
			mData->title.c_str(),
			nullptr, nullptr);

		if (!mWindow)
		{
			Log::Console(LogLevel::Fatal, "[GlfwWindow] Window creation failed.");
			return false;
		}

		glfwSetWindowUserPointer(mWindow, mData);
		RegisterCallbacks();

		return true;
	}

	void GlfwWindow::RegisterCallbacks()
	{
		glfwSetWindowCloseCallback(mWindow, [](GLFWwindow* window)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				EventWindowClose event;
				data.eventCallback(event);
			});

		glfwSetWindowFocusCallback(mWindow, [](GLFWwindow* window, int32 focused)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

				if (focused)
				{
					EventWindowFocusEnter event;
					data.eventCallback(event);
				}
				else
				{
					EventWindowFocusExit event;
					data.eventCallback(event);
				}
			});

		glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int32 width, int32 height)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				data.width = static_cast<uint32>(width);
				data.height = static_cast<uint32>(height);

				EventWindowResize event(width, height);

				if (data.eventCallback)
					data.eventCallback(event);
			});

		glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mods)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

				switch (action)
				{
					case GLFW_PRESS:   { EventInputKeyboardPress event(key);   data.eventCallback(event); break; }
					case GLFW_RELEASE: { EventInputKeyboardRelease event(key); data.eventCallback(event); break; }
					case GLFW_REPEAT:  { EventInputKeyboardRepeat event(key);  data.eventCallback(event); break; }
				}
			});

		glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int32 button, int32 action, int32 mods)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

				switch (action)
				{
					case GLFW_PRESS:   { EventInputMousePress event(button);   data.eventCallback(event); break; }
					case GLFW_RELEASE: { EventInputMouseRelease event(button); data.eventCallback(event); break; }
				}
			});

		glfwSetScrollCallback(mWindow, [](GLFWwindow* window, float64 xOffset, float64 yOffset)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				EventInputMouseScroll event(static_cast<float32>(xOffset), static_cast<float32>(yOffset));
				data.eventCallback(event);
			});

		glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, float64 xPos, float64 yPos)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				EventInputMouseMove event(static_cast<float32>(xPos), static_cast<float32>(yPos));
				data.eventCallback(event);
			});
	}

	void GlfwWindow::Show()
	{
		glfwShowWindow(mWindow);
	}

	void GlfwWindow::PollEvents()
	{
		glfwPollEvents();
	}

	bool GlfwWindow::ShouldClose() const
	{
		return glfwWindowShouldClose(mWindow);
	}

	void GlfwWindow::SetDisplayTitle(const std::string& title)
	{
		if (mWindow)
			glfwSetWindowTitle(mWindow, title.c_str());
	}

	void GlfwWindow::SetResizable(bool enable)
	{
		if (mWindow)
			glfwSetWindowAttrib(mWindow, GLFW_RESIZABLE, enable ? GLFW_TRUE : GLFW_FALSE);
	}

	void GlfwWindow::SetIcon(const std::string& filePath)
	{
		int32 width = 0, height = 0, channels = 0;
		byte* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 4); // Force RGBA.

		if (!pixels)
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[GlfwWindow] Failed to load icon image: '{}'.", filePath));
			return;
		}

		if (mIcon)
		{
			stbi_image_free(mIcon->pixels);
			delete mIcon;
		}

		mIcon = new GLFWimage();
		mIcon->width = width;
		mIcon->height = height;
		mIcon->pixels = pixels;

		if (mWindow)
			glfwSetWindowIcon(mWindow, 1, mIcon);
	}

	bool GlfwWindow::IsKeyPressed(int32 keyCode) const
	{
		return glfwGetKey(mWindow, keyCode) == GLFW_PRESS;
	}

	bool GlfwWindow::IsKeyReleased(int32 keyCode) const
	{
		return glfwGetKey(mWindow, keyCode) == GLFW_RELEASE;
	}

	void* GlfwWindow::GetNativeHandle() const
	{
		return mWindow;
	}
}
