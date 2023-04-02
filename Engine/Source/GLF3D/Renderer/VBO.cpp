#include "Core.h"
#include "VBO.h"

VBO::VBO(std::vector<Vertex> Vertices)
{
	glGenBuffers(1, &m_Id);
	glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertex), Vertices.data(), GL_STATIC_DRAW);
}

VBO::~VBO()
{
	glDeleteBuffers(1, &m_Id);
}

void VBO::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_Id);
}

void VBO::Unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
