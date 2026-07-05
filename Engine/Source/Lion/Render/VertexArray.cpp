#include "Engine.h"
#include "VertexArray.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Render/OpenGL/OpenGLVertexArray.h>

namespace Lion
{
	Reference<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLVertexArray>();

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[VertexArray] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
