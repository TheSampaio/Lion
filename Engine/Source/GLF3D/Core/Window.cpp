#include "Core.h"
#include "Window.h"

Window::Window()
    : m_Id(nullptr)
{
    m_Title = "Window";
    m_Size = {800, 600};
    m_GLVersion = {0, 0};
    m_BackgroundColor = {0.0f, 0.0f, 0.0f};

    // Initialize and verifying if GLFW was initialized
    if (!glfwInit())
    {
        Debug::Log::Error("Failed to initialize GLFW.");
        exit(EXIT_FAILURE);
    }
}

Window::~Window()
{
    // Destroying window's process
    glfwDestroyWindow(m_Id);

    // Finilizing GLFW
    glfwTerminate();
}

bool Window::Create()
{
    // Creates a window
    m_Id = glfwCreateWindow(m_Size[0], m_Size[1], m_Title.c_str(), nullptr, nullptr);

    // Verifying if window was initialized
    if (!m_Id)
    {
        Debug::Log::Error("Failed to create a window.");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Set OpenGL's context
    glfwMakeContextCurrent(m_Id);

    // Loads GLEW
    if (!gladLoadGL())
    {
        Debug::Log::Error("Failed to load GLAD.");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Creates a viewport
    glViewport(0, 0, GetSize()[0], GetSize()[1]);

    // Changes background's color
    glClearColor(m_BackgroundColor[0], m_BackgroundColor[1], m_BackgroundColor[2], 1.0f);

    return (m_Id) ? true : false;
}

void Window::SetOpenGLVersion(unsigned int Major, unsigned int Minor)
{
    m_GLVersion = { Major, Minor };

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, Major);             // Sets OpenGL's major version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, Minor);             // Sets OpenGL's minor version
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Sets OpenGL's CORE mode
}
