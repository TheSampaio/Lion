#include "Core.h"
#include "Shader.h"

#include "../Core/Application.h"

// Reference to the engine debugger
Debug& Shader::s_Debug = *Application::s_Debug;

Shader::Shader(const char* VertexShaderFile, const char* FragmentShaderFile)
{
	CompileShader(VertexShaderFile, m_VertexShader, GL_VERTEX_SHADER);
	CompileShader(FragmentShaderFile, m_FragmentShader, GL_FRAGMENT_SHADER);
	CreateShaderProgram(m_VertexShader, m_FragmentShader);
}

Shader::~Shader()
{
	glDeleteShader(m_VertexShader);
	glDeleteShader(m_FragmentShader);
	glDeleteProgram(m_Id);
}

std::string Shader::LoadShader(const char* FilePath)
{
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
		s_Debug.Log(Error, "Failed to read \"", false);
		s_Debug.Log(None, FilePath, false);
		s_Debug.Log(None, "\"", false, true);
	}

	return Content;
}

void Shader::CompileShader(const char* FilePath, GLuint& ShaderId, GLenum ShaderType)
{
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

			s_Debug.Log(Error, "[ERROR] Failed to compile shader ", false);
			s_Debug.Log(Error, ShaderInfoLog.c_str());
		}
	}
}

void Shader::CreateShaderProgram(GLuint VextexShader, GLuint FragmentShader)
{
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

			s_Debug.Log(Error, "[ERROR] Failed to link shader program ", false);
			s_Debug.Log(Error, ProgramInfoLog.c_str());
		}
	}

	// Eliminate what we need anymore
	glDetachShader(m_Id, m_VertexShader);
	glDetachShader(m_Id, m_FragmentShader);
}