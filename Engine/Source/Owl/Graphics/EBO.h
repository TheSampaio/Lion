#ifndef OWL_EBO_H
#define OWL_EBO_H

struct OWL_API Index
{
	// Attributes
	GLuint X, Y, Z;
};

class OWL_API EBO
{
public:
	EBO(std::vector<Index> Indices);
	~EBO();

	// Main methods
	void Bind();
	void Unbind();

private:
	// Attributes
	GLuint m_Id;
};

#endif
