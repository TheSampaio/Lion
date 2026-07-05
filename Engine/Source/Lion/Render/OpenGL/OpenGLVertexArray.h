#pragma once

#include <Lion/Render/VertexArray.h>

namespace Lion
{
	// OpenGL vertex array object.
	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		~OpenGLVertexArray() override;

		void Bind() const override;
		void Unbind() const override;

		void AddVertexBuffer(const Reference<VertexBuffer>& vertexBuffer) override;
		void SetIndexBuffer(const Reference<IndexBuffer>& indexBuffer) override;

		const Reference<IndexBuffer>& GetIndexBuffer() const override { return mIndexBuffer; }

	private:
		uint32 mId = 0;
		uint32 mAttributeIndex = 0;

		std::vector<Reference<VertexBuffer>> mVertexBuffers;
		Reference<IndexBuffer> mIndexBuffer;
	};
}
