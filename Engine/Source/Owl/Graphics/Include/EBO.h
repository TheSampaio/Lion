#ifndef OWL_EBO_H
#define OWL_EBO_H

#include "../Math/Include/Index.h";

class OWL_API EBO
{
public:
	EBO(std::vector<Index> Indices);
	~EBO();

	// Main methods
	inline void Bind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id); }
	inline void Unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

private:
	// Attributes
	GLuint m_Id;
};

#endif
