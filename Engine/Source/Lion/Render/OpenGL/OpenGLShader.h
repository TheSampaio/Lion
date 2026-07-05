#pragma once

#include <Lion/Render/Shader.h>

namespace Lion
{
	// OpenGL/GLSL implementation of the Shader abstraction.
	class OpenGLShader : public Shader
	{
	public:
		explicit OpenGLShader(const std::string& filePath);
		~OpenGLShader() override;

		void Bind() const override;
		void Unbind() const override;

		void SetInt(const std::string& name, int32 value) override;
		void SetIntArray(const std::string& name, const int32* values, uint32 count) override;
		void SetFloat(const std::string& name, float32 value) override;
		void SetFloat3(const std::string& name, const glm::vec3& value) override;
		void SetMat4(const std::string& name, const glm::mat4& value) override;

	private:
		uint32 mProgram = 0;

		struct Source
		{
			std::string vertex;
			std::string fragment;
		};

		int32 GetUniformLocation(const std::string& name) const;

		static Source Parse(const std::string& filePath);
		static uint32 Compile(uint32 type, const std::string& source);
		static uint32 Link(uint32 vertexShader, uint32 fragmentShader);
	};
}
