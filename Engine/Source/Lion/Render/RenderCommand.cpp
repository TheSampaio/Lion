#include "Engine.h"
#include "RenderCommand.h"

namespace Lion
{
	void RenderCommand::SetShaderInt(uint32 shaderProgram, const char8* name, int32 value)
	{
		glUniform1i(glGetUniformLocation(shaderProgram, name), value);
	}

	void RenderCommand::SetShaderFloat(uint32 shaderProgram, const char8* name, float32 value)
	{
		glUniform1f(glGetUniformLocation(shaderProgram, name), value);
	}

	void RenderCommand::SetShaderFloat3(uint32 shaderProgram, const char8* name, float32 value1, float32 value2, float32 value3)
	{
		glUniform3f(glGetUniformLocation(shaderProgram, name), value1, value2, value3);
	}

	void RenderCommand::SetShaderMatrix4(uint32 shaderProgram, const char8* name, glm::mat4 value)
	{
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name), 1, GL_FALSE, glm::value_ptr(value));
	}

	void RenderCommand::BindTexture2D(uint32 texture)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	void RenderCommand::Clear(uint32 mask)
	{
		glClear(mask);
	}

	void RenderCommand::CreateViewport(int32 x, int32 y, uint32 width, uint32 height)
	{
		glViewport(x, y, width, height);
	}

	void RenderCommand::ClearColor(float32 red, float32 green, float32 blue, float32 alpha)
	{
		glClearColor(red, green, blue, alpha);
	}

	void RenderCommand::DrawIndexedQuads(int32 count)
	{
		glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_INT, nullptr);
	}
}
