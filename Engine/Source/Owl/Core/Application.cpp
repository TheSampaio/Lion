#include "Core.h"
#include "Application.h"

// initialize static pointers
std::unique_ptr<Debug>    Application::s_Debug = nullptr;
std::unique_ptr<Renderer> Application::s_Renderer = nullptr;
std::unique_ptr<Window>   Application::s_Window = nullptr;

Application::Application()
{
    // Initialize all libraries
    Init();

    // Creates unique pointers
    s_Debug = std::make_unique<Debug>();
    s_Window = std::make_unique<Window>();
    s_Renderer = std::make_unique<Renderer>();
}

Application::~Application()
{
    // Finalize glfw
    glfwTerminate();
}

// Runs the game
bool Application::Run()
{
    // Creates a window
    if (!s_Window->Create())
    {
        s_Debug->Log(Error, "Failed to create a window.");
        return EXIT_FAILURE;
    }

    if (!s_Renderer->Init())
    {
        s_Debug->Log(Error, "Failed to initialize renderer.");
        return EXIT_FAILURE;
    }

    // Runs main loop (Gameloop)
    return Loop();
}

// Main loop
bool Application::Loop()
{
    // Start timer and game
    // TODO: s_Timer->Start()
    this->Start();

    do
    {
        // Process all window's events
        s_Window->ProcessEvents();

        // Update game and clear all buffers
        this->Update(1.0f);
        s_Renderer->ClearBuffers(*s_Window);

        // Draw everything and swap buffers
        this->Draw();
        s_Renderer->SwapBuffers(*s_Window);

    } while (!s_Window->Close());

    // Finish game
    this->Finalize();
    return false;
}

void Application::Init()
{
    // Initilizes GLFW and log it if failed
    if (!glfwInit())
    {
        s_Debug->Log(Error, "Failed to initialize GLFW.");
        exit(EXIT_FAILURE);
    }
}
