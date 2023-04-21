#include "Core.h"
#include "Shader.h"

#include "../Core/Debug.h"

Shader::Shader(const char* VertexShaderFile, const char* FragmentShaderFile)
{
	// Compile the vertex and fragment shaders and create a shader program
	CompileShader(VertexShaderFile, m_VertexShader, GL_VERTEX_SHADER);
	CompileShader(FragmentShaderFile, m_FragmentShader, GL_FRAGMENT_SHADER);
	CreateShaderProgram(m_VertexShader, m_FragmentShader);
}

Shader::~Shader()
{
	// Deletes the shaders and shader program created
	glDeleteShader(m_VertexShader);
	glDeleteShader(m_FragmentShader);
	glDeleteProgram(m_Id);
}

std::string Shader::LoadShader(const char* FilePath)
{
	// Get the content from a text file
	std::fstream File;
	std::string Source;
	std::string Content;

	File.open(FilePath, std::ios::in);

	if (File.is_open())
	{
		while (std::getline(File, Source))
		{
			Content.append(Source + "\n");
		}

		File.close();
	}

	if (Content.empty())
	{
		Debug::Log(Error, "Failed to read \"", false, false);
		Debug::Log(None, FilePath, false, false);
		Debug::Log(None, "\"", true, false);
	}

	return Content;
}

void Shader::CompileShader(const char* FilePath, GLuint& ShaderId, GLenum ShaderType)
{
	// Compiles a shader
	std::string Source = LoadShader(FilePath);
	const char* pSource = Source.c_str();

	ShaderId = glCreateShader(ShaderType);
	glShaderSource(ShaderId, 1, &pSource, nullptr);
	glCompileShader(ShaderId);

	// ShaderID must be a compiled shader
	GLint Result = GL_TRUE;
	glGetShaderiv(ShaderId, GL_COMPILE_STATUS, &Result);

	if (Result == GL_FALSE)
	{
		// Get log's length 
		GLint InfoLogLength = 0;
		glGetShaderiv(ShaderId, GL_INFO_LOG_LENGTH, &InfoLogLength);

		if (InfoLogLength > 0)
		{
			std::string ShaderInfoLog(InfoLogLength, '\0');
			glGetShaderInfoLog(ShaderId, InfoLogLength, nullptr, &ShaderInfoLog[0]);

			Debug::Log(Error, "Failed to compile shader ", false, false);
			Debug::Log(None, ShaderInfoLog.c_str(), true, false);
		}
	}
}

void Shader::CreateShaderProgram(GLuint VextexShader, GLuint FragmentShader)
{
	// Creates a shader program and attach the created shaders to the shader program
	m_Id = glCreateProgram();
	glAttachShader(m_Id, VextexShader);
	glAttachShader(m_Id, FragmentShader);
	glLinkProgram(m_Id);

	// Check if the link was successful
	GLint Result = GL_TRUE;
	glGetProgramiv(m_Id, GL_LINK_STATUS, &Result);

	if (Result == GL_FALSE)
	{
		GLint InfoLogLength = 0;
		glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &InfoLogLength);

		if (InfoLogLength > 0)
		{
			std::string ProgramInfoLog(InfoLogLength, '\0');
			glGetProgramInfoLog(m_Id, InfoLogLength, nullptr, &ProgramInfoLog[0]);

			Debug::Log(Error, "Failed to link shader program ", false);
			Debug::Log(None, ProgramInfoLog.c_str(), true, false);
		}
	}

	// Deatachs the created shaders from the shader program
	glDetachShader(m_Id, m_VertexShader);
	glDetachShader(m_Id, m_FragmentShader);
}