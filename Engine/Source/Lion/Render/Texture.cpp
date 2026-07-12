#include "Engine.h"
#include "Texture.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Render/OpenGL/OpenGLTexture.h>

namespace Lion
{
	Reference<Texture> Texture::Create(const std::string& filePath, TextureFilter filter)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLTexture>(filePath, filter);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[Texture] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
