#pragma once

namespace Lion
{
	class RenderCommand
	{
	public:
		RenderCommand() = delete;

		// Sets an integer uniform in the given shader program.
		static void SetShaderInt(uint32 shaderProgram, const char8* name, int32 value);

		// Sets a float uniform in the given shader program.
		static void SetShaderFloat(uint32 shaderProgram, const char8* name, float32 value);

		// Sets a vec3 (3-component float vector) uniform in the given shader program.
		static void SetShaderFloat3(uint32 shaderProgram, const char8* name, float32 value1, float32 value2, float32 value3);

		// Sets a mat4 (4x4 float matrix) uniform in the given shader program.
		static void SetShaderMatrix4(uint32 shaderProgram, const char8* name, glm::mat4 value);

		// Binds a 2D texture to the current texture unit.
		static void BindTexture2D(uint32 texture);

		// Clears the color or depth buffer based on the provided mask (e.g., GL_COLOR_BUFFER_BIT).
		static void Clear(uint32 mask);

		// Creates a viewport for rendering with the specified position and size.
		static void CreateViewport(int32 x, int32 y, uint32 width, uint32 height);

		// Clears the screen with the specified clear color (red, green, blue, alpha).
		static void ClearColor(float32 red, float32 green, float32 blue, float32 alpha);

		// Draws the specified number of indexed quads.
		static void DrawIndexedQuads(int32 count);
	};
}
