#pragma once

#include <Lion/Render/GraphicsContext.h>

struct GLFWwindow;

namespace Lion
{
	// OpenGL rendering context backed by a GLFW window.
	class OpenGLContext : public GraphicsContext
	{
	public:
		explicit OpenGLContext(void* windowHandle);

		bool Init() override;
		void SwapBuffers() override;
		void SetVerticalSync(bool enable) override;

		std::string GetDeviceName() const override;
		std::string GetApiVersion() const override;

	private:
		GLFWwindow* mWindowHandle;
	};
}
