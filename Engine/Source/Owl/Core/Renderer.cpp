#include "Core.h"
#include "Renderer.h"

#include "Application.h"

// Reference to the engine debugger
Debug&  Renderer::s_Debug = *Application::s_Debug;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

bool Renderer::Init()
{
	return true;
}

void Renderer::Draw(std::vector<Index>& Indices)
{
	glDrawElements(GL_TRIANGLES, Indices.size() * 3, GL_UNSIGNED_INT, nullptr);
}
