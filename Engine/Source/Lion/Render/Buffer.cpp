#include "Engine.h"
#include "Buffer.h"

#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Render/OpenGL/OpenGLBuffer.h>

namespace Lion
{
	uint32 ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:  return 4;
			case ShaderDataType::Float2: return 4 * 2;
			case ShaderDataType::Float3: return 4 * 3;
			case ShaderDataType::Float4: return 4 * 4;
			case ShaderDataType::Int:    return 4;
			case ShaderDataType::Int2:   return 4 * 2;
			case ShaderDataType::Int3:   return 4 * 3;
			case ShaderDataType::Int4:   return 4 * 4;
			case ShaderDataType::None:   break;
		}

		Log::Console(LogLevel::Error, "[Buffer] Unknown ShaderDataType.");
		return 0;
	}

	BufferElement::BufferElement(ShaderDataType type, const std::string& name, bool normalized)
		: name(name), type(type), size(ShaderDataTypeSize(type)), offset(0), normalized(normalized)
	{
	}

	uint32 BufferElement::GetComponentCount() const
	{
		switch (type)
		{
			case ShaderDataType::Float:  return 1;
			case ShaderDataType::Float2: return 2;
			case ShaderDataType::Float3: return 3;
			case ShaderDataType::Float4: return 4;
			case ShaderDataType::Int:    return 1;
			case ShaderDataType::Int2:   return 2;
			case ShaderDataType::Int3:   return 3;
			case ShaderDataType::Int4:   return 4;
			case ShaderDataType::None:   break;
		}

		return 0;
	}

	BufferLayout::BufferLayout(std::initializer_list<BufferElement> elements)
		: mElements(elements)
	{
		CalculateOffsetsAndStride();
	}

	void BufferLayout::CalculateOffsetsAndStride()
	{
		uint32 offset = 0;
		mStride = 0;

		for (BufferElement& element : mElements)
		{
			element.offset = offset;
			offset += element.size;
			mStride += element.size;
		}
	}

	Reference<VertexBuffer> VertexBuffer::Create(uint32 size)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLVertexBuffer>(size);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[VertexBuffer] Selected graphics backend is not supported.");
				return nullptr;
		}
	}

	Reference<VertexBuffer> VertexBuffer::Create(const void* vertices, uint32 size)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLVertexBuffer>(vertices, size);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[VertexBuffer] Selected graphics backend is not supported.");
				return nullptr;
		}
	}

	Reference<IndexBuffer> IndexBuffer::Create(const uint32* indices, uint32 count)
	{
		switch (RendererAPI::GetAPI())
		{
			case GraphicsAPI::OpenGL: return MakeReference<OpenGLIndexBuffer>(indices, count);

			case GraphicsAPI::Vulkan:
			case GraphicsAPI::None:
			default:
				Log::Console(LogLevel::Fatal, "[IndexBuffer] Selected graphics backend is not supported.");
				return nullptr;
		}
	}
}
