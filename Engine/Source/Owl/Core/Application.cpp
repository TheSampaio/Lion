#include "Core.h"
#include "Application.h"

// Initializes static pointers
std::unique_ptr<Input>    Application::s_Input = nullptr;
std::unique_ptr<Renderer> Application::s_Renderer = nullptr;
std::unique_ptr<Timer>    Application::s_Timer = nullptr;
std::unique_ptr<Window>   Application::s_Window = nullptr;

float Application::s_FrameTime = 0.0f;

Application::Application()
{
    // Initializes all libraries
    this->Init();

    // Creates unique pointers
    s_Input = std::make_unique<Input>();
    s_Renderer = std::make_unique<Renderer>();
    s_Timer = std::make_unique<Timer>();
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
    s_Timer->Start();
    this->Start();

    do
    {
        // Calculates frametime
        s_FrameTime = FrameTimeMonitor();

        // Process all window's events
        s_Input->ProcessEvents();
        s_Input->ProcessCallbacks();

        // Update game and clear all buffers
        this->Update(s_FrameTime);
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

float Application::FrameTimeMonitor()
{
#ifdef OWL_DEBUG
    static float TotalTime = 0.0f;
    static unsigned FrameCount = 0;
#endif // OWL_DEBUG

    s_FrameTime =+ s_Timer->Reset();

#ifdef OWL_DEBUG
    TotalTime += s_FrameTime;
    FrameCount++;

    if (TotalTime >= 1.0f)
    {
        std::stringstream Header;

        // Fixes header precision
        Header << std::fixed;
        Header.precision(1);

        // Sets new windows's title
        Header << s_Window->GetTitle().c_str() << " | FPS: " << FrameCount << " | MS: " << s_FrameTime * 1000.0f;
        s_Window->SetTitle(Header.str().c_str());

        // Resets frame count and total time
        FrameCount = 0;
        TotalTime -= 1.0f;
    }
#endif // OWL_DEBUG

    return s_FrameTime;
}
