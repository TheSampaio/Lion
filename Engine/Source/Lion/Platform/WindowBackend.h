#pragma once

namespace Lion
{
	class Event;

	// Mutable window state shared between the Window facade and its platform backend.
	//
	// The backend receives a pointer to this struct and updates it from native callbacks (for
	// example, the live size on resize) and invokes the event callback. Keeping it in one place
	// lets the engine read window state without touching any windowing library.
	struct WindowData
	{
		std::string title;
		uint32 width = 0;
		uint32 height = 0;
		std::function<void(Event&)> eventCallback;
		bool resizable = true;
		bool maximized = false;
	};

	// Platform windowing backend.
	//
	// Every native windowing call (creation, event pumping, input polling, presentation
	// surface handle) lives behind this interface. The engine never calls a windowing library
	// directly, so the implementation (GLFW today) can be swapped without touching engine code.
	class WindowBackend
	{
	public:
		virtual ~WindowBackend() = default;

		// Creates the native window and wires up event callbacks into the given data block.
		virtual bool Initialize(WindowData* data) = 0;

		// Makes the window visible.
		virtual void Show() = 0;

		// Pumps pending native events.
		virtual void PollEvents() = 0;

		// Whether the user requested the window to close.
		virtual bool ShouldClose() const = 0;

		// Programmatically requests the window to close (e.g. an editor "Exit" action).
		virtual void RequestClose() = 0;

		// Updates the text shown in the title bar (does not change the stored base title).
		virtual void SetDisplayTitle(const std::string& title) = 0;

		// Allows or forbids the user to resize the window at runtime.
		virtual void SetResizable(bool enable) = 0;

		// Loads and applies a window icon from an image file.
		virtual void SetIcon(const std::string& filePath) = 0;

		// Immediate keyboard state, using engine key codes.
		virtual bool IsKeyPressed(int32 keyCode) const = 0;
		virtual bool IsKeyReleased(int32 keyCode) const = 0;

		// Opaque native window handle, consumed by the graphics backend to create its surface.
		virtual void* GetNativeHandle() const = 0;

		// Instantiates the windowing backend for the current platform.
		static Scope<WindowBackend> Create();
	};
}
