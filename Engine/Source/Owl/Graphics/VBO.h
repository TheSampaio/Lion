#ifndef OWL_VBO_H
#define OWL_VBO_H

#include "../Math/Vertex.h";

class OWL_API VBO
{
public:
	VBO(std::vector<Vertex> Vertices);
	~VBO();

	// Main methods
	void Bind();
	void Unbind();

private:
	// Attributes
	GLuint m_Id;
};

#endif