#include "Core.h"
#include "EBO.h"

EBO::EBO(std::vector<Index> Indices)
{
	glGenBuffers(1, &m_Id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(Index), Indices.data(), GL_STATIC_DRAW);
}

EBO::~EBO()
{
	glDeleteBuffers(1, &m_Id);
}

void EBO::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
}

void EBO::Unbind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
