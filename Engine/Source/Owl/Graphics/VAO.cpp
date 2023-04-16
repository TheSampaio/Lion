#include "Core.h"
#include "VAO.h"

VAO::VAO()
{
	// Create and activates a vertex array object
	glGenVertexArrays(1, &m_Id);
	glBindVertexArray(m_Id);
}

VAO::~VAO()
{
	// Deletes the created vertex array object from memory
	glDeleteVertexArrays(1, &m_Id);
}

void VAO::AttribPointer(GLuint Layout, GLint Size, GLsizei Stride, GLenum Offset, GLenum Type, GLboolean Normalized)
{
	// Setups the VAO's layouts
	glVertexAttribPointer(Layout, Size, Type, Normalized, Stride, reinterpret_cast<void*>(Offset));
	glEnableVertexAttribArray(Layout);
}

