#ifndef OWL_RENDERER_H
#define OWL_RENDERER_H

#include "../Renderer/Shader.h"
#include "../Renderer/VAO.h"
#include "../Renderer/EBO.h"

class OWL_API Renderer
{
public:
	Renderer();
	~Renderer();

	// Main methods
	bool Init();
	void Draw(std::vector<Index>& IndexBuffer);

	// Friend classes
	friend class Application;

private:
	// Static attributes
	static class Debug& s_Debug;
};

#endif // !GLF3D_RENDERER_H
