#include "Core.h"
#include "Renderer.h"

#include "Application.h"

#include "../Graphics/Shader.h"
#include "../Graphics/VAO.h"

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
