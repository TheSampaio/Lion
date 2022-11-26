#include "Core.h"
#include "Application.h"

Game*   Application::s_Game = nullptr;
Window* Application::s_Window = nullptr;

Application::Application()
{
    s_Window = new Window;
}

Application::~Application()
{
    delete s_Game;
    delete s_Window;
}

// Starts the game
bool Application::Start(Game* World)
{
    s_Game = World;

    if (!s_Window->Create())
    {
        Debug::Log::Error("Failed to create a Window");
        return EXIT_FAILURE;
    }

    return Run();
}

// Runs the engine
bool Application::Run()
{
    s_Game->Start();

    do
    {
        // Events
        s_Window->ProcessEvents();

        float Temporary = 1.0f;
        s_Game->Update(Temporary);
        s_Window->ClearBuffers();

        s_Game->Draw();
        s_Window->SwapBuffers();

    } while (!s_Window->Close());

    s_Game->End();

    return false;
}