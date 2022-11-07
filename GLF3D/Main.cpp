#include "PCH.h"
#include "Window.h"

// Static variables
static Window s_Window;

// Functions's prototypes
void ResizeViewport(GLFWwindow* Window, int Width, int Height);

int main()
{    
    // Setups and create a window
    s_Window.SetTitle("OpenGL Window");
    s_Window.SetSize(600, 400);
    s_Window.SetOpenGLVersion(4, 0);
    s_Window.Create();

    // Sighs window's callbacks
    glfwSetFramebufferSizeCallback(s_Window.GetId(), ResizeViewport);

    // Main loop
    while (!glfwWindowShouldClose(s_Window.GetId())) // Windows's loop
    {
        s_Window.SetBackgroundColor(0.80f, 0.85f, 0.90f);

        // TODO: Draw Here!

        s_Window.ProcessEvents();
        s_Window.SwapBuffers();
    }
           
    return 0;
}

// Resizes window's viewport
void ResizeViewport(GLFWwindow* Window, int Width, int Height)
{
    s_Window.SetSize(Width, Height);
    glViewport(0, 0, Width, Height);
}
