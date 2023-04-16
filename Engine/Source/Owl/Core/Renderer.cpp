#include "Core.h"
#include "Renderer.h"

#include "Application.h"

#include "../Graphics/Shader.h"
#include "../Graphics/VAO.h"

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

void Renderer::ClearBuffers()
{
	std::array<float, 3> BackgroundColor = {
		static_cast<float>(Application::GetWindow()->GetBackgroundColor()[0] / 255.0f),
		static_cast<float>(Application::GetWindow()->GetBackgroundColor()[1] / 255.0f),
		static_cast<float>(Application::GetWindow()->GetBackgroundColor()[2] / 255.0f)
	};

	// Clear buffers and colorizes window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(BackgroundColor[0], BackgroundColor[1], BackgroundColor[2], 1.0f);

	m_ShaderProgram->Activate();
}

void Renderer::SwapBuffers()
{
	// Set V-Sync and Swap buffers
	glfwSwapInterval(m_SynchronizationMode);
	glfwSwapBuffers(Application::GetWindow()->GetId());
}
