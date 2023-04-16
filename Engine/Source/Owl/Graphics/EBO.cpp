#include "Core.h"
#include "EBO.h"

EBO::EBO(std::vector<Index> Indices)
{
	// Creates and setup an element buffer object
	glGenBuffers(1, &m_Id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(Index), Indices.data(), GL_STATIC_DRAW);
}

EBO::~EBO()
{
	// Deletes the created element buffer
	glDeleteBuffers(1, &m_Id);
}

void EBO::Bind()
{
	// Activates the created element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
}

void EBO::Unbind()
{
	// Deactivates the created element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}