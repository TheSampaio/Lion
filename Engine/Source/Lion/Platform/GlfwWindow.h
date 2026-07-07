#pragma once

#include <Lion/Platform/WindowBackend.h>

struct GLFWwindow;
struct GLFWimage;

namespace Lion
{
	// GLFW implementation of the windowing backend.
	//
	// This is the single translation unit that talks to GLFW. The window is created with a
	// client API matching the selected graphics backend (an OpenGL context for OpenGL, or no
	// context for Vulkan), which is what makes the engine ready for a future Vulkan backend.
	class GlfwWindow : public WindowBackend
	{
	public:
		GlfwWindow() = default;
		~GlfwWindow() override;

		bool Initialize(WindowData* data) override;
		void Show() override;
		void PollEvents() override;
		bool ShouldClose() const override;
		void RequestClose() override;

		void SetDisplayTitle(const std::string& title) override;
		void SetResizable(bool enable) override;
		void SetIcon(const std::string& filePath) override;

		bool IsKeyPressed(int32 keyCode) const override;
		bool IsKeyReleased(int32 keyCode) const override;

		void* GetNativeHandle() const override;

	private:
		GLFWwindow* mWindow = nullptr;
		GLFWimage* mIcon = nullptr;
		WindowData* mData = nullptr;  // Owned by the Window facade; not freed here.

		void RegisterCallbacks();
	};
}
