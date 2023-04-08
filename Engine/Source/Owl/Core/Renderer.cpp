#include "Core.h"
#include "Renderer.h"
#include "Application.h"

#include "../Graphics/VAO.h"

// Reference to the engine debugger
Debug& Renderer::s_Debug = *Application::s_Debug;

Renderer::Renderer()
	: m_ShaderProgram(nullptr)
{
	// Initialize all attributes
	m_SynchronizationMode = ESynchronizationMode::Full;
}

Renderer::~Renderer()
{
	// Releases dynamically allocated memory
	delete m_ShaderProgram;
}

bool Renderer::Init()
{
	// Create shader program
	m_ShaderProgram = new Shader("../../Owl/Engine/Shaders/DefaultVert.glsl", "../../Owl/Engine/Shaders/DefaultFrag.glsl");

	//return true;
	return (m_ShaderProgram) ? true : false;
}

void Renderer::Draw(std::vector<Index>& Indices)
{
	// Draw elements command
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(Indices.size() * 3), GL_UNSIGNED_INT, nullptr);
}

void Renderer::ClearBuffers(Window& Window)
{
	// Clear buffers and colorizes window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(static_cast<GLfloat>(Window.GetBackgroundColor()[0] / 255.0f), static_cast<GLfloat>(Window.GetBackgroundColor()[1] / 255.0f), static_cast<GLfloat>(Window.GetBackgroundColor()[2] / 255.0f), 1.0f);

	m_ShaderProgram->Activate();
}

void Renderer::SwapBuffers(Window& Window)
{
	// Set V-Sync and Swap buffers
	glfwSwapInterval(m_SynchronizationMode);
	glfwSwapBuffers(Window.GetId());
}
