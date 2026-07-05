#pragma once

namespace Lion
{
	// Abstraction over the platform rendering context bound to a window.
	//
	// It owns the connection between the window and the graphics backend (context creation,
	// buffer presentation and vertical synchronization).
	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		// Makes the context current and loads the backend entry points.
		virtual bool Init() = 0;

		// Presents the back buffer to the screen.
		virtual void SwapBuffers() = 0;

		// Enables or disables vertical synchronization.
		virtual void SetVerticalSync(bool enable) = 0;

		// Human-readable name of the GPU driving the context.
		virtual std::string GetDeviceName() const = 0;

		// Human-readable version string of the active graphics backend.
		virtual std::string GetApiVersion() const = 0;

		// Creates the context implementation for the selected backend, bound to windowHandle.
		static Scope<GraphicsContext> Create(void* windowHandle);
	};
}
