#include "Core.h"
#include "Include/Renderer.h"

#include "Include/Application.h"

#include "../Graphics/Include/Shader.h"
#include "../Graphics/Include/VAO.h"

Renderer::Renderer()
	: m_ShaderProgram(nullptr), bWireframe(false)
{
	// Initialize all attributes
	m_SynchronizationMode = ESynchronizationMode::Full;
	m_VersionGL = { 3, 3 };
}

Renderer::~Renderer()
{
	// Releases dynamically allocated memory
	delete m_ShaderProgram;
}

bool Renderer::Init()
{
	// Gets graphics card's vendor
	Debug::Log(Information, "Vendor: ", false, false);
	Debug::Log(None, reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

	// Gets graphics card's model
	Debug::Log(Information, "Graphics card: ", false, false);
	Debug::Log(None, reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

	// Gets OpenGL's version
	Debug::Log(Information, "OpenGL version: ", false, false);
	Debug::Log(None, reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	// Gets GLSL's version
	Debug::Log(Information, "GLSL version: ", false, false);
	Debug::Log(None, reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)), false);
	Debug::Log(None, "===", false);

	// Create shader program
	m_ShaderProgram = new Shader("../../Owl/Engine/Shaders/DefaultVert.glsl", "../../Owl/Engine/Shaders/DefaultFrag.glsl");

	// Returns true if exists a default shader program;
	return (m_ShaderProgram) ? true : false;
}

void Renderer::Draw(std::vector<Index>& Indices)
{
	// Set-ups primitive that is going to be drawn
	auto Primitive = (!bWireframe) ? GL_TRIANGLES : GL_LINES;

	// Draw primitives using indices
	glDrawElements(Primitive, static_cast<GLsizei>(Indices.size() * 3), GL_UNSIGNED_INT, nullptr);
}

void Renderer::ClearBuffers()
{
	// Casts window's colour from "unsigned short" to "GLfloat"
	std::array<float, 3> BackgroundColour = {
		static_cast<GLfloat>(Application::GetWindow()->GetBackgroundColour()[0] / 255.0f),
		static_cast<GLfloat>(Application::GetWindow()->GetBackgroundColour()[1] / 255.0f),
		static_cast<GLfloat>(Application::GetWindow()->GetBackgroundColour()[2] / 255.0f)
	};

	// Clear buffers and colourizes the window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(BackgroundColour[0], BackgroundColour[1], BackgroundColour[2], 1.0f);

	// Activates the default shader program
	m_ShaderProgram->Activate();
}

void Renderer::SwapBuffers()
{
	// Set V-Sync and Swap buffers
	glfwSwapInterval(m_SynchronizationMode);
	glfwSwapBuffers(Application::GetWindow()->GetId());
}
