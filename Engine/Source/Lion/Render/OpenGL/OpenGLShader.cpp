#include "Engine.h"
#include "OpenGLShader.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Vault.h>

namespace Lion
{
	OpenGLShader::OpenGLShader(const std::string& filePath)
	{
		const Source source = Parse(filePath);

		const uint32 vertexShader = Compile(GL_VERTEX_SHADER, source.vertex);
		const uint32 fragmentShader = Compile(GL_FRAGMENT_SHADER, source.fragment);

		mProgram = Link(vertexShader, fragmentShader);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(mProgram);
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(mProgram);
	}

	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::SetInt(const std::string& name, int32 value)
	{
		glUniform1i(GetUniformLocation(name), value);
	}

	void OpenGLShader::SetIntArray(const std::string& name, const int32* values, uint32 count)
	{
		glUniform1iv(GetUniformLocation(name), static_cast<GLsizei>(count), values);
	}

	void OpenGLShader::SetFloat(const std::string& name, float32 value)
	{
		glUniform1f(GetUniformLocation(name), value);
	}

	void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
		glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
	}

	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
	}

	int32 OpenGLShader::GetUniformLocation(const std::string& name) const
	{
		return glGetUniformLocation(mProgram, name.c_str());
	}

	OpenGLShader::Source OpenGLShader::Parse(const std::string& filePath)
	{
		enum class Stage
		{
			None = -1,
			Vertex = 0,
			Fragment = 1,
		};

		std::ifstream file(ResolveResourcePath(filePath), std::ios::binary);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[OpenGLShader] Failed to locate shader file: '{}'.", filePath));
			return {};
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string content = buffer.str();

		// A shipped shader is sealed and one read from the project is not, and this does not have to know
		// which: the content says so itself. It is one pass over the file, once, at load.
		content = Vault::Unseal(content);

		// Normalize line endings so plain and de-obfuscated sources parse identically.
		content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

		std::stringstream source[2];
		Stage stage = Stage::None;
		std::string line;
		std::istringstream stream(content);

		while (std::getline(stream, line))
		{
			if (line.find("#shader") != std::string::npos)
			{
				if (line.find("vertex") != std::string::npos)
					stage = Stage::Vertex;

				else if (line.find("fragment") != std::string::npos)
					stage = Stage::Fragment;
			}
			else if (stage != Stage::None)
			{
				source[static_cast<int32>(stage)] << line << '\n';
			}
		}

		return { source[0].str(), source[1].str() };
	}

	uint32 OpenGLShader::Compile(uint32 type, const std::string& source)
	{
		const uint32 id = glCreateShader(type);
		const GLchar* const content = source.c_str();

		glShaderSource(id, 1, &content, nullptr);
		glCompileShader(id);

		int32 result = GL_FALSE;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);

		if (result == GL_FALSE)
		{
			int32 length = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

			std::vector<GLchar> message(length);
			glGetShaderInfoLog(id, length, &length, message.data());

			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[OpenGLShader] Failed to compile {} shader:\n{}",
				(type == GL_VERTEX_SHADER) ? "vertex" : "fragment", message.data()));

			glDeleteShader(id);
			return 0;
		}

		return id;
	}

	uint32 OpenGLShader::Link(uint32 vertexShader, uint32 fragmentShader)
	{
		const uint32 program = glCreateProgram();

		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		int32 isLinked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

		if (isLinked == GL_FALSE)
		{
			int32 length = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

			std::vector<GLchar> message(length);
			glGetProgramInfoLog(program, length, &length, message.data());

			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[OpenGLShader] Failed to link shader program:\n{}", message.data()));

			glDeleteProgram(program);
			return 0;
		}

		// Shaders are linked into the program and no longer needed on their own.
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return program;
	}
}
