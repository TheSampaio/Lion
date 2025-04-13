#pragma once

namespace Lion
{
	class RenderCommand
	{
	public:
		RenderCommand() = delete;

		// Sets an integer uniform variable in the specified shader program.
		static void SetUniform1i(uint32 shaderProgram, const char8* name, int32 value);

		// Sets a float uniform variable in the specified shader program.
		static void SetUniform1f(uint32 shaderProgram, const char8* name, float32 value);

		// Sets a vec3 (3-component float vector) uniform variable in the specified shader program.
		static void SetUniform3f(uint32 shaderProgram, const char8* name, float32 value1, float32 value2, float32 value3);

		// Sets a mat4 (4x4 float matrix) uniform variable in the specified shader program.
		static void SetUniformMatrix4fv(uint32 shaderProgram, const char8* name, glm::mat4 value);
	};
}
