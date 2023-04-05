#ifndef OWL_EBO_H
#define OWL_EBO_H

struct OWL_API Index
{
	GLuint X, Y, Z;
};

class OWL_API EBO
{
public:
	EBO(std::vector<Index> Indices);
	~EBO();

	void Bind();
	void Unbind();

private:
	GLuint m_Id;
};

#endif
