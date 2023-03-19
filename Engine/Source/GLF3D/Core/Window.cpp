#include "Core.h"
#include "Window.h"

#include "Application.h"

// Reference to the engine debugger
Debug& Window::s_Debug = *Application::s_Debug;

Window::Window()
	: m_Id(nullptr), m_Monitor(nullptr)
{
	// Initilizes GLFW and log it if failed
	if (!glfwInit())
	{
		s_Debug.Log(Error, "Failed to initialize GLFW.");
		exit(EXIT_FAILURE);
	}

	// Setup default attributes
	m_Monitor = glfwGetPrimaryMonitor();

	// Initializes title, V-Sync and display mode
	m_Title = "Window";
	m_SynchronizationMode = ESynchronizationMode::Disabled;
	m_DisplayMode = EDisplayMode::Windowed;

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

	// Finalize glfw
	glfwTerminate();
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
		m_Id = glfwCreateWindow(m_Size[0], m_Size[1], m_Title.data(), m_Monitor, nullptr);

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
		m_Id = glfwCreateWindow(m_Size[0], m_Size[1], m_Title.data(), nullptr, nullptr);

		if (m_Id)
		{
			m_Position = { (m_Screen[0] / 2) - (m_Size[0] / 2), (m_Screen[1] / 2) - (m_Size[1] / 2) };
			glfwSetWindowPos(m_Id, m_Position[0], m_Position[1]);
			glfwMaximizeWindow(m_Id);
		}
	}

	// Creates a OpenGL's constext for the window
	glfwMakeContextCurrent(m_Id);

	// Loads GLAD and log it if failed
	if (!gladLoadGL())
	{
		s_Debug.Log(Error, "Failed to load GLAD.");
		return EXIT_FAILURE;
	}

	// Creates a viewport for the window
	glViewport(0, 0, m_Size[0], m_Size[1]); // Creating a viewport

	// Return TRUE if exists a window
	return (m_Id) ? true : false;
}

bool Window::Close()
{
	// Return close window message
	return glfwWindowShouldClose(m_Id);
}

void Window::ProcessEvents()
{
	// Process all window events
	glfwPollEvents();
}

void Window::ClearBuffers()
{
	// Clear buffers and colorizes window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(static_cast<GLfloat>(m_BackgroundColor[0] / 255.0f), static_cast<GLfloat>(m_BackgroundColor[1] / 255.0f), static_cast<GLfloat>(m_BackgroundColor[2] / 255.0f), 1.0f);
}

void Window::SwapBuffers()
{
	// Set V-Sync and Swap buffers
	glfwSwapInterval(m_SynchronizationMode);
	glfwSwapBuffers(m_Id);
}