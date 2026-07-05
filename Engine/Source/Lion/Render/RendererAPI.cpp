#include "Engine.h"
#include "RendererAPI.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/OpenGL/OpenGLRendererAPI.h>

namespace Lion
{
	// The active backend is fixed at compile time until a runtime selection mechanism exists.
	GraphicsAPI RendererAPI::sAPI = GraphicsAPI::OpenGL;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (sAPI)
		{
			case GraphicsAPI::OpenGL: return MakeScope<OpenGLRendererAPI>();

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[RendererAPI] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
