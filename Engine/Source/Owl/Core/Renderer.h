#ifndef OWL_RENDERER_H
#define OWL_RENDERER_H

#include "Window.h"

#include "../Math/Vertex.h"
#include "../Math/Index.h"

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

	// Get methods
	inline ESynchronizationMode GetSynchronizationMode()    { return m_SynchronizationMode; }
	inline std::array<unsigned short, 2> GetOpenGLVersion() { return m_VersionGL; }

	// Set methods
	inline void SetSynchronizationMode(ESynchronizationMode SynchronizationMode) { m_SynchronizationMode = SynchronizationMode; }
	inline void SetWireframeMode(bool Enable)                                    { bWireframe = Enable; }
	inline void SetOpenGLVersion(unsigned short Major, unsigned short Minor)     { m_VersionGL = { Major, Minor }; }

	// Friend classes
	friend class Application;

private:
	// Attributes
	ESynchronizationMode m_SynchronizationMode;
	class Shader* m_ShaderProgram;
	bool bWireframe;

	std::array<unsigned short, 2> m_VersionGL;

	// Main methods
	void ClearBuffers();
	void SwapBuffers();
};

#endif // !GLF3D_RENDERER_H
