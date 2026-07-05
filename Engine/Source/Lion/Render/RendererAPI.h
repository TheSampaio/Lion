#pragma once

namespace Lion
{
	// Rendering backends the engine can target. Only OpenGL is implemented today; Vulkan is
	// reserved so a second backend can be dropped in without touching high-level render code.
	enum class GraphicsAPI
	{
		None = 0,
		OpenGL,
		Vulkan,
	};

	// Low-level, backend-specific rendering commands.
	//
	// This is the single seam every graphics API implements. High-level code never calls the
	// underlying API directly; it goes through RenderCommand, which forwards to a RendererAPI.
	class RendererAPI
	{
	public:
		virtual ~RendererAPI() = default;

		// Sets up backend-wide render state (blending, defaults, ...).
		virtual void Init() = 0;

		// Defines the region of the framebuffer to render into.
		virtual void SetViewport(int32 x, int32 y, uint32 width, uint32 height) = 0;

		// Sets the color used to clear the framebuffer.
		virtual void SetClearColor(float32 red, float32 green, float32 blue, float32 alpha) = 0;

		// Clears the currently bound framebuffer.
		virtual void Clear() = 0;

		// Issues an indexed draw for the currently bound geometry.
		virtual void DrawIndexed(uint32 indexCount) = 0;

		// Returns the backend currently selected at compile time.
		static GraphicsAPI GetAPI() { return sAPI; }

		// Instantiates the RendererAPI implementation for the selected backend.
		static Scope<RendererAPI> Create();

	private:
		static GraphicsAPI sAPI;
	};
}
