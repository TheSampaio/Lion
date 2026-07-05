#pragma once

namespace Lion
{
	class RendererAPI;

	// Static, backend-agnostic front-end for issuing low-level render commands.
	//
	// RenderCommand simply forwards to the active RendererAPI. High-level systems (Renderer,
	// Graphics) talk to this class and never touch a graphics API directly.
	class RenderCommand
	{
	public:
		RenderCommand() = delete;

		// Creates the backend RendererAPI and initializes its render state.
		static void Init();

		// Releases the backend RendererAPI.
		static void Shutdown();

		// Sets the viewport rectangle.
		static void SetViewport(int32 x, int32 y, uint32 width, uint32 height);

		// Sets the framebuffer clear color.
		static void SetClearColor(float32 red, float32 green, float32 blue, float32 alpha);

		// Clears the framebuffer.
		static void Clear();

		// Issues an indexed draw for the currently bound geometry.
		static void DrawIndexed(uint32 indexCount);

	private:
		static Scope<RendererAPI> sRendererAPI;
	};
}
