#ifndef OWL_VBO_H
#define OWL_VBO_H

struct OWL_API Vertex
{
	// Attributes
	glm::vec4 Position, Color;
};

class VBO
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