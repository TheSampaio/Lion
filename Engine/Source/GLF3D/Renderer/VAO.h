#ifndef GLF3D_VAO_H
#define GLF3D_VAO_H

class GLF3D_API VAO
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


#endif // !GLF3D_VAO_H