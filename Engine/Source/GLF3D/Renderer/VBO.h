#ifndef GLF3D_VBO_H
#define GLF3D_VBO_H

struct GLF3D_API Vertex
{
	glm::vec4 Position, Color;
};

class GLF3D_API VBO
{
public:
	VBO(std::vector<Vertex> Vertices);
	~VBO();

	void Bind();
	void Unbind();

private:
	GLuint m_Id;
};

#endif // !GLF3D_VBO_H