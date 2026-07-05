#pragma once

#include <Lion/Render/RendererAPI.h>

namespace Lion
{
	// OpenGL implementation of the RendererAPI seam.
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		void Init() override;
		void SetViewport(int32 x, int32 y, uint32 width, uint32 height) override;
		void SetClearColor(float32 red, float32 green, float32 blue, float32 alpha) override;
		void Clear() override;
		void DrawIndexed(uint32 indexCount) override;
	};
}
