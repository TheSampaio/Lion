#include "Engine.h"
#include "Shader.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Render/OpenGL/OpenGLShader.h>

namespace Lion
{
	Reference<Shader> Shader::Create(const std::string& filePath)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLShader>(filePath);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[Shader] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
