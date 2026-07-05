#include "Engine.h"
#include "OpenGLVertexArray.h"

#include <Lion/Render/Buffer.h>

namespace Lion
{
	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
				return GL_FLOAT;

			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
				return GL_INT;

			case ShaderDataType::None:
			default:
				return 0;
		}
	}

	OpenGLVertexArray::OpenGLVertexArray()
	{
		glGenVertexArrays(1, &mId);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &mId);
	}

	void OpenGLVertexArray::Bind() const
	{
		glBindVertexArray(mId);
	}

	void OpenGLVertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

	void OpenGLVertexArray::AddVertexBuffer(const Reference<VertexBuffer>& vertexBuffer)
	{
		glBindVertexArray(mId);
		vertexBuffer->Bind();

		const BufferLayout& layout = vertexBuffer->GetLayout();

		for (const BufferElement& element : layout)
		{
			glEnableVertexAttribArray(mAttributeIndex);
			glVertexAttribPointer(
				mAttributeIndex,
				static_cast<GLint>(element.GetComponentCount()),
				ShaderDataTypeToOpenGLBaseType(element.type),
				element.normalized ? GL_TRUE : GL_FALSE,
				static_cast<GLsizei>(layout.GetStride()),
				reinterpret_cast<const void*>(static_cast<uintptr_t>(element.offset)));

			mAttributeIndex++;
		}

		mVertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArray::SetIndexBuffer(const Reference<IndexBuffer>& indexBuffer)
	{
		glBindVertexArray(mId);
		indexBuffer->Bind();

		mIndexBuffer = indexBuffer;
	}
}
