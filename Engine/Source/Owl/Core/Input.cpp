#include "Core.h"
#include "Input.h"

#include "Application.h"

// Initializes static attributes
bool Input::m_bPressed = false;

bool Input::GetKeyPressed(EKeyCode KeyCode)
{
	// Verifies if a key IS pressed
	return (glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_PRESS) ? true : false;
}

bool Input::GetKeyReleased(EKeyCode KeyCode)
{
	// Verifies if a key IS pressed
	return (glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_RELEASE) ? true : false;
}

bool Input::GetKeyTapped(EKeyCode KeyCode)
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
	// Process all window's events
	glfwPollEvents();
}

void Input::ProcessCallbacks()
{
	// Process all window's callbacks
	glfwSetFramebufferSizeCallback(Application::GetWindow()->GetId(), FramebufferCallback);
}

void Input::FramebufferCallback(GLFWwindow* Id, int Width, int Height)
{
	// Resizes window's viewport
	Application::GetWindow()->SetSize(static_cast<unsigned short>(Width), static_cast<unsigned short>(Height));
	glViewport(0, 0, Application::GetWindow()->GetSize()[0], Application::GetWindow()->GetSize()[1]);
}
