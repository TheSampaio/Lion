#ifndef OWL_VBO_H
#define OWL_VBO_H

struct OWL_API Vertex
{
	glm::vec4 Position, Color;
};

class OWL_API VBO
{
public:
	VBO(std::vector<Vertex> Vertices);
	~VBO();

	void Bind();
	void Unbind();

private:
	GLuint m_Id;
};

#endif