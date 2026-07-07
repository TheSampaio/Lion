#include "Engine.h"
#include "Framebuffer.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Render/OpenGL/OpenGLFramebuffer.h>

namespace Lion
{
	Reference<Framebuffer> Framebuffer::Create(const FramebufferSpecification& specification)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLFramebuffer>(specification);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[Framebuffer] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
