#ifndef GLF3D_EBO_H
#define GLF3D_EBO_H

struct GLF3D_API Index
{
	GLuint X, Y, Z;
};

class GLF3D_API EBO
{
public:
	EBO(std::vector<Index> Indices);
	~EBO();

	void Bind();
	void Unbind();

private:
	GLuint m_Id;
};

#endif // !GLF3D_EBO_H
