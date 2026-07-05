#pragma once

namespace Lion
{
	// Scalar/vector types a vertex attribute can hold.
	enum class ShaderDataType
	{
		None = 0,
		Float, Float2, Float3, Float4,
		Int, Int2, Int3, Int4,
	};

	// Size, in bytes, of a single value of the given shader data type.
	uint32 ShaderDataTypeSize(ShaderDataType type);

	// A single attribute inside a vertex layout (e.g. position, color, ...).
	struct BufferElement
	{
		std::string name;
		ShaderDataType type = ShaderDataType::None;
		uint32 size = 0;
		uint32 offset = 0;
		bool normalized = false;

		BufferElement() = default;
		BufferElement(ShaderDataType type, const std::string& name, bool normalized = false);

		// Number of scalar components in the attribute (e.g. Float3 -> 3).
		uint32 GetComponentCount() const;
	};

	// Ordered set of vertex attributes describing the memory layout of a vertex.
	class BufferLayout
	{
	public:
		BufferLayout() = default;
		BufferLayout(std::initializer_list<BufferElement> elements);

		uint32 GetStride() const { return mStride; }
		const std::vector<BufferElement>& GetElements() const { return mElements; }

		std::vector<BufferElement>::const_iterator begin() const { return mElements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return mElements.end(); }

	private:
		std::vector<BufferElement> mElements;
		uint32 mStride = 0;

		void CalculateOffsetsAndStride();
	};

	// GPU buffer holding vertex data.
	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		// Uploads (a prefix of) the buffer contents; used for dynamic streaming.
		virtual void SetData(const void* data, uint32 size) = 0;

		virtual const BufferLayout& GetLayout() const = 0;
		virtual void SetLayout(const BufferLayout& layout) = 0;

		// Creates an empty dynamic buffer of the given byte size.
		static Reference<VertexBuffer> Create(uint32 size);

		// Creates a static buffer initialized with vertices.
		static Reference<VertexBuffer> Create(const void* vertices, uint32 size);
	};

	// GPU buffer holding 32-bit indices.
	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual uint32 GetCount() const = 0;

		static Reference<IndexBuffer> Create(const uint32* indices, uint32 count);
	};
}
