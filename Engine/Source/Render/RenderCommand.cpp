#include "Engine.h"
#include "RenderCommand.h"

namespace Lion
{
	void RenderCommand::SetUniform1i(GLuint shaderProgram, const GLchar* name, GLint value)
	{
		glUniform1i(glGetUniformLocation(shaderProgram, name), value);
	}

	void RenderCommand::SetUniform1f(GLuint shaderProgram, const GLchar* name, GLfloat value)
	{
		glUniform1f(glGetUniformLocation(shaderProgram, name), value);
	}

	void RenderCommand::SetUniform3f(GLuint shaderProgram, const GLchar* name, GLfloat value1, GLfloat value2, GLfloat value3)
	{
		glUniform3f(glGetUniformLocation(shaderProgram, name), value1, value2, value3);
	}

	void RenderCommand::SetUniformMatrix4fv(GLuint shaderProgram, const GLchar* name, glm::mat4 value)
	{
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name), 1, GL_FALSE, glm::value_ptr(value));
	}
}
