#include "Core.h"
#include "Input.h"

#include "Application.h"

// Initializes static attributes
int Input::s_KeyCode = 0;
bool Input::s_bPressed = false;

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
		s_KeyCode = KeyCode;
		s_bPressed = true;
	}

	if ((s_bPressed) && (glfwGetKey(Application::s_Window->GetId(), KeyCode) == GLFW_RELEASE) && (KeyCode == s_KeyCode))
	{
		s_KeyCode = 0;
		s_bPressed = false;
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
