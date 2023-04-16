#include "Core.h"
#include "Input.h"

#include "Application.h"

bool Input::m_bPressed = false;

bool Input::GetKeyPressed(int KeyCode)
{
	// Verifies if a key IS pressed
	return (glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_PRESS) ? true : false;
}

bool Input::GetKeyReleased(int KeyCode)
{
	// Verifies if a key IS pressed
	return (glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_RELEASE) ? true : false;
}

bool Input::GetKeyTapped(int KeyCode)
{
	// Verifies if a key WAS pressed (Tapped)
	if (glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_PRESS)
	{
		m_bPressed = true;
		return false;
	}

	if ((glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_RELEASE) && (m_bPressed))
	{
		m_bPressed = false;
		return true;
	}

	return false;
}

void Input::ProcessEvents()
{
	// Process all window events
	glfwPollEvents();
}
