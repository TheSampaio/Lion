#include "Core.h"
#include "Include/VBO.h"

VBO::VBO(std::vector<Vertex> Vertices)
{
	// Creates and setup a vertex buffer object
	glGenBuffers(1, &m_Id);
	glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertex), Vertices.data(), GL_STATIC_DRAW);
}

VBO::~VBO()
{
	// Deletes the created vertex array object from memory
	glDeleteBuffers(1, &m_Id);
}
