#pragma once

namespace Lion
{
	// GPU shader program abstraction.
	//
	// A shader is created from a single ".glsl" file that contains both the vertex and the
	// fragment stages, separated by "#shader vertex" / "#shader fragment" tags.
	class Shader
	{
	public:
		virtual ~Shader() = default;

		// Binds / unbinds the program for subsequent draw calls.
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		// Uniform setters.
		virtual void SetInt(const std::string& name, int32 value) = 0;
		virtual void SetIntArray(const std::string& name, const int32* values, uint32 count) = 0;
		virtual void SetFloat(const std::string& name, float32 value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

		// Creates a shader from a combined ".glsl" source file for the selected backend.
		static Reference<Shader> Create(const std::string& filePath);
	};
}
