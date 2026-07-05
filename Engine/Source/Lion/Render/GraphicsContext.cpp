#include "Engine.h"
#include "GraphicsContext.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Render/OpenGL/OpenGLContext.h>

namespace Lion
{
	Scope<GraphicsContext> GraphicsContext::Create(void* windowHandle)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL:
				return MakeScope<OpenGLContext>(windowHandle);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[GraphicsContext] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
