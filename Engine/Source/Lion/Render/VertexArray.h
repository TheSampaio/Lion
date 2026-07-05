#pragma once

namespace Lion
{
	class VertexBuffer;
	class IndexBuffer;

	// Binds together vertex buffers (with their layouts) and an index buffer.
	//
	// This maps to an OpenGL VAO, but the concept is kept abstract so other backends can
	// provide their own equivalent (or a no-op) without changing high-level render code.
	class VertexArray
	{
	public:
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddVertexBuffer(const Reference<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const Reference<IndexBuffer>& indexBuffer) = 0;

		virtual const Reference<IndexBuffer>& GetIndexBuffer() const = 0;

		static Reference<VertexArray> Create();
	};
}
