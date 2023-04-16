#include "Core.h"
#include "Window.h"

#include "Debug.h"

Window::Window()
	: m_Id(nullptr), m_Monitor(nullptr)
{
	// Setup default attributes
	m_Monitor = glfwGetPrimaryMonitor();

	// Initializes title, V-Sync and display mode
	m_Title = "Window";
	m_DisplayMode = EDisplayMode::Windowed;

	m_bMaximize = true;

	// Initializes window's size and center position
	m_Size = { 800, 600 };
	m_Center = { static_cast<unsigned short>(m_Size[0] / 2), static_cast<unsigned short>(m_Size[1] / 2) };

	// Initializes screen's size and window's position
	glfwGetMonitorWorkarea(m_Monitor, 0, 0, &m_Screen[0], &m_Screen[1]);
	m_Position = { (m_Screen[0] / 2) - (m_Size[0] / 2), (m_Screen[1] / 2) - (m_Size[1] / 2) };

	// Initializes backeground color and OpenGL version
	m_BackgroundColor = { 0, 0, 0 };
	m_OpenGLVersion = { 3, 3 };
}

Window::~Window()
{
	// Destroy window from memory
	glfwDestroyWindow(m_Id);
}

bool Window::Close(bool Force)
{
	if (!Force)
	{
		return glfwWindowShouldClose(m_Id);
	}
	
	else
	{
		// Forces window to close
		glfwSetWindowShouldClose(m_Id, Force);
		return false;
	}
}

bool Window::Create()
{
	// Setup OpenGL's version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, m_OpenGLVersion[0]);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, m_OpenGLVersion[1]);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// If FULLSCREEN mode
	if (m_DisplayMode == EDisplayMode::Fullscreen)
	{
		// Create a window and set its position (Fullscreen mode)
		m_Id = glfwCreateWindow(m_Size[0], m_Size[1], m_Title.c_str(), m_Monitor, nullptr);

		if (m_Id)
		{
			m_Position = { 0, 0 };
			glfwSetWindowPos(m_Id, m_Position[0], m_Position[1]);
		}
	}

	// If WINDOWED mode
	else
	{
		// Create a window and set its position (Windowed mode)
		if (m_DisplayMode == EDisplayMode::Windowed)
		{
			m_Id = glfwCreateWindow(m_Size[0], m_Size[1], m_Title.c_str(), nullptr, nullptr);

			if (m_Id)
			{
				m_Position = { (m_Screen[0] / 2) - (m_Size[0] / 2), (m_Screen[1] / 2) - (m_Size[1] / 2) };
				glfwSetWindowPos(m_Id, m_Position[0], m_Position[1]);
			}
		}

		// Create a window and set its position (Borderless mode)
		else
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

			m_Size = { static_cast<unsigned short>(m_Screen[0]), static_cast<unsigned short>(m_Screen[1] + 40) };
			m_Id = glfwCreateWindow(m_Size[0], m_Size[1], m_Title.c_str(), nullptr, nullptr);

			if (m_Id)
			{
				m_Position = { 0, 0 };
				glfwSetWindowPos(m_Id, m_Position[0], m_Position[1]);
			}
		}
	}

	// Creates a OpenGL's constext for the window
	glfwMakeContextCurrent(m_Id);

	// Loads GLAD and log it if failed
	if (!gladLoadGL())
	{
		Debug::Log(Error, "Failed to load GLAD.", true, true);
		return EXIT_FAILURE;
	}

	// Creates a viewport for the window
	if (m_DisplayMode == EDisplayMode::Windowed)
	{
		if (m_bMaximize)
		{
			glfwMaximizeWindow(m_Id);
			glViewport(0, 0, m_Screen[0], m_Screen[1]);
		}
	}

	else
	{
		glViewport(0, 0, m_Size[0], m_Size[1]);
	}

	// Return TRUE if exists a window
	return (m_Id) ? true : false;
}
