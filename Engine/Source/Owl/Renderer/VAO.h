#ifndef OWL_VAO_H
#define OWL_VAO_H

class OWL_API VAO
{
public:
	VAO();
	~VAO();

	// Main methods
	void Bind();
	void Unbind();
	void AttribPointer(GLuint Layout, GLint Size, GLsizei Stride, GLenum Offset, GLenum Type = GL_FLOAT, GLboolean Normalized = GL_FALSE);

private:
	GLuint m_Id;
};


#endif