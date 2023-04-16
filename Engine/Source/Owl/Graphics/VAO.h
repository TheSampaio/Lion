#ifndef OWL_VAO_H
#define OWL_VAO_H

class OWL_API VAO
{
public:
	VAO();
	~VAO();

	// Main methods	
	void AttribPointer(GLuint Layout, GLint Size, GLsizei Stride, GLenum Offset, GLenum Type = GL_FLOAT, GLboolean Normalized = GL_FALSE);

	inline void Bind() { glBindVertexArray(m_Id); }
	inline void Unbind() { glBindVertexArray(0); }

private:
	// Attributes
	GLuint m_Id;
};


#endif