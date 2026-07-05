#include "Engine.h"
#include "OpenGLRendererAPI.h"

namespace Lion
{
	void OpenGLRendererAPI::Init()
	{
		// Enable straight-alpha transparency for sprites.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void OpenGLRendererAPI::SetViewport(int32 x, int32 y, uint32 width, uint32 height)
	{
		glViewport(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	}

	void OpenGLRendererAPI::SetClearColor(float32 red, float32 green, float32 blue, float32 alpha)
	{
		glClearColor(red, green, blue, alpha);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(uint32 indexCount)
	{
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
	}
}
