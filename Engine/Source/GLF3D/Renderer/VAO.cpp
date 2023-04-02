#include "Core.h"
#include "VAO.h"

VAO::VAO()
{
	glGenVertexArrays(1, &m_Id);
	glBindVertexArray(m_Id);
}

VAO::~VAO()
{
	glDeleteVertexArrays(1, &m_Id);
}

void VAO::Bind()
{
	glBindVertexArray(m_Id);
}

void VAO::Unbind()
{
	glBindVertexArray(0);
}

void VAO::AttribPointer(GLuint Layout, GLint Size, GLsizei Stride, GLenum Offset, GLenum Type, GLboolean Normalized)
{
	glVertexAttribPointer(Layout, Size, Type, Normalized, Stride, reinterpret_cast<void*>(Offset));
	glEnableVertexAttribArray(Layout);
}

