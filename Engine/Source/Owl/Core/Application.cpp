#include "Core.h"
#include "Application.h"

// initialize static pointers
std::unique_ptr<Input>    Application::s_Input = nullptr;
std::unique_ptr<Renderer> Application::s_Renderer = nullptr;
std::unique_ptr<Window>   Application::s_Window = nullptr;

Application::Application()
{
    // Initializes all libraries
    this->Init();

    // Creates unique pointers
    s_Input = std::make_unique<Input>();
    s_Renderer = std::make_unique<Renderer>();
    s_Window = std::make_unique<Window>();
}

Application::~Application()
{
    // Finalizes glfw
    glfwTerminate();
}

// Runs the game
bool Application::Run()
{
    // Creates a window
    if (!s_Window->Create())
    {
        Debug::Log(Error, "Failed to create a window.");
        return EXIT_FAILURE;
    }

    if (!s_Renderer->Init())
    {
        Debug::Log(Error, "Failed to initialize renderer.");
        return EXIT_FAILURE;
    }

    // Runs main loop (Gameloop)
    return Loop();
}

// Main loop (Game loop)
bool Application::Loop()
{
    // Start timer and game
    // TODO: s_Timer->Start()
    this->Start();

    do
    {
        // Process all window's events
        s_Input->ProcessEvents();

        // Update game and clear all buffers
        this->Update(1.0f);
        s_Renderer->ClearBuffers();

        // Draw everything and swap buffers
        this->Draw();
        s_Renderer->SwapBuffers();

    } while (!s_Window->Close());

    // Finish game
    this->Finalize();
    return false;
}

void Application::Init()
{
    // IF Windows platform THEN set console title
#ifdef _WIN
    system("TITLE Owl Engine");
#endif // _WIN

    // Initilizes GLFW and log it if failed
    if (!glfwInit())
    {
        Debug::Log(Error, "Failed to initialize GLFW.");
        exit(EXIT_FAILURE);
    }
}
