#ifndef OWL_VBO_H
#define OWL_VBO_H

#include "../Math/Include/Vertex.h";

class OWL_API VBO
{
public:
	VBO(std::vector<Vertex> Vertices);
	~VBO();

	// Main methods
	inline void Bind() { glBindBuffer(GL_ARRAY_BUFFER, m_Id);}
	inline void Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

private:
	// Attributes
	GLuint m_Id;
};

#endif