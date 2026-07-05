#pragma once

#include <Lion/Render/Buffer.h>

namespace Lion
{
	// OpenGL vertex buffer object.
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		explicit OpenGLVertexBuffer(uint32 size);
		OpenGLVertexBuffer(const void* vertices, uint32 size);
		~OpenGLVertexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void SetData(const void* data, uint32 size) override;

		const BufferLayout& GetLayout() const override { return mLayout; }
		void SetLayout(const BufferLayout& layout) override { mLayout = layout; }

	private:
		uint32 mId = 0;
		BufferLayout mLayout;
	};

	// OpenGL element (index) buffer object.
	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(const uint32* indices, uint32 count);
		~OpenGLIndexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		uint32 GetCount() const override { return mCount; }

	private:
		uint32 mId = 0;
		uint32 mCount = 0;
	};
}
