#pragma once

namespace Lion
{
	class RenderCommand
	{
	public:
		RenderCommand() = delete;

		// Sets an integer uniform variable in the specified shader program.
		static void SetUniform1i(GLuint shaderProgram, const GLchar* name, GLint value);

		// Sets a float uniform variable in the specified shader program.
		static void SetUniform1f(GLuint shaderProgram, const GLchar* name, GLfloat value);

		// Sets a vec3 (3-component float vector) uniform variable in the specified shader program.
		static void SetUniform3f(GLuint shaderProgram, const GLchar* name, GLfloat value1, GLfloat value2, GLfloat value3);

		// Sets a mat4 (4x4 float matrix) uniform variable in the specified shader program.
		static void SetUniformMatrix4fv(GLuint shaderProgram, const GLchar* name, glm::mat4 value);
	};
}
