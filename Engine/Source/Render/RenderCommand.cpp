#include "Engine.h"
#include "RenderCommand.h"

namespace Lion
{
	void RenderCommand::SetUniform1i(uint32 shaderProgram, const char8* name, int32 value)
	{
		glUniform1i(glGetUniformLocation(shaderProgram, name), value);
	}

	void RenderCommand::SetUniform1f(uint32 shaderProgram, const char8* name, float32 value)
	{
		glUniform1f(glGetUniformLocation(shaderProgram, name), value);
	}

	void RenderCommand::SetUniform3f(uint32 shaderProgram, const char8* name, float32 value1, float32 value2, float32 value3)
	{
		glUniform3f(glGetUniformLocation(shaderProgram, name), value1, value2, value3);
	}

	void RenderCommand::SetUniformMatrix4fv(uint32 shaderProgram, const char8* name, glm::mat4 value)
	{
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name), 1, GL_FALSE, glm::value_ptr(value));
	}
}
