#ifndef OWL_RENDERER_H
#define OWL_RENDERER_H

#include "Window.h"

#include "../Graphics/Shader.h"
#include "../Graphics/Mesh.h"

// Enumerate window's V-Sync modes
enum ESynchronizationMode
{
	Disabled = 0,
	Full,
	Half
};

class OWL_API Renderer
{
public:
	Renderer();
	~Renderer();

	// Delete copy constructor and assignment operator
	Renderer(const Renderer&) = delete;
	Renderer operator=(const Renderer&) = delete;

	// Main methods
	bool Init();
	void Draw(std::vector<Index>& Indices);

	// Set methods
	inline void SetSynchronizationMode(ESynchronizationMode SynchronizationMode) { m_SynchronizationMode = SynchronizationMode; }

	// Get methods
	inline ESynchronizationMode GetSynchronizationMode() { return m_SynchronizationMode; }

	// Friend classes
	friend class Application;

private:
	// Attributes
	Shader* m_ShaderProgram;
	ESynchronizationMode m_SynchronizationMode;

	// Main methods
	void ClearBuffers();
	void SwapBuffers();
};

#endif // !GLF3D_RENDERER_H
