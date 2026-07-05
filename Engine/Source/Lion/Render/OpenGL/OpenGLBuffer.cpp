#include "Engine.h"
#include "OpenGLBuffer.h"

namespace Lion
{
	// ---- Vertex buffer -----------------------------------------------------------------

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32 size)
	{
		glGenBuffers(1, &mId);
		glBindBuffer(GL_ARRAY_BUFFER, mId);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(const void* vertices, uint32 size)
	{
		glGenBuffers(1, &mId);
		glBindBuffer(GL_ARRAY_BUFFER, mId);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &mId);
	}

	void OpenGLVertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, mId);
	}

	void OpenGLVertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::SetData(const void* data, uint32 size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	// ---- Index buffer ------------------------------------------------------------------

	OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32* indices, uint32 count)
		: mCount(count)
	{
		glGenBuffers(1, &mId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * count, indices, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &mId);
	}

	void OpenGLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mId);
	}

	void OpenGLIndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}
