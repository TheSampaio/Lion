#include "Core.h"
#include "Application.h"

// initialize static pointers
std::unique_ptr<Debug>  Application::s_Debug = nullptr;
std::unique_ptr<Window> Application::s_Window = nullptr;

Application::Application()
{
    // Creates unique pointers
    s_Debug = std::make_unique<Debug>();
    s_Window = std::make_unique<Window>();
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
        s_Window->ClearBuffers();

        // Draw everything and swap buffers
        this->DrawCall();
        // TODO: s_Renderer->Draw();
        s_Window->SwapBuffers();

    } while (!s_Window->Close());

    // Finish game
    this->Finalize();
    return false;
}