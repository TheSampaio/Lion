#ifndef GLF3D_SHADER_H
#define GLF3D_SHADER_H

class GLF3D_API Shader
{
public:
	Shader(const char* VertexShaderFile, const char* FragmentShaderFile);
	~Shader();

	// Main methods
	inline void Activate() { glUseProgram(m_Id); }

	// Get methods
	inline GLuint& GetProgram()        { return m_Id; }
	inline GLuint& GetVertexShader()   { return m_VertexShader; }
	inline GLuint& GetFragmentShader() { return m_FragmentShader; }

private:
	GLuint m_Id,
		m_VertexShader,
		m_FragmentShader;

	// Main methods
	std::string LoadShader(const char* FilePath);
	void CompileShader(const char* FilePath, GLuint& ShaderId, GLenum ShaderType);
	void CreateShaderProgram(GLuint VextexShader, GLuint FragmentShader);

	// Static attributes
	static class Debug& s_Debug;
};

#endif // !GLF3D_SHADER_H
