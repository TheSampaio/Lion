#include "Engine.h"
#include "Header/Application.h"

#include "Header/Window.h"
#include "../Event/Header/Debug.h"
#include "../Event/Header/Input.h"
#include "../Event/Header/Time.h"
#include "../Logic/Header/Game.h"
#include "../Logic/Header/Timer.h"
#include "../Render/Header/Graphics.h"

bool owl::Application::s_bPaused = false;

owl::Application::Application()
	: Game(nullptr)
{
	// Changes console's title (Only in debug mode)
#ifdef WL_DEBUG
	std::system("TITLE Owl Engine");
#endif // WL_DEBUG
}

owl::Application::~Application()
{
	// Deletes the hame
	delete Game;
}

int owl::Application::IRun(class Game* Level)
{
	// Creates the game
	Game = Level;

	// Creates the window
	if (!Window::GetInstance().Create())
	{
		Debug::Message(Error, "Failed to create the window.");
		return false;
	}

	// Initializes graphics
	if (!Graphics::GetInstance().Initialize())
	{
		Debug::Message(Error, "Failed to initialize graphics.");
		return false;
	}

	// TODO: Initializes the renderer

	// Game loop
	timeBeginPeriod(1); // Changes the default "Sleep()" functions' precision
	int State = Loop(); // Runs the engine's game loop
	timeBeginPeriod(1); // Restores the default "Sleep()" functions' precision

	// Returns game loop's state
	return State;
}

int owl::Application::Loop()
{
	// Starts the Time's timer and initializes the game
	Time::GetInstance().Timer->Start();
	Game->OnStart();

	// Store all window's events
	MSG Messages = { 0 };

	do
	{
		// Process all window's events per frame (Real time function)
		if (PeekMessage(&Messages, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Messages);
			DispatchMessage(&Messages);
		}

		else
		{
			// Engine's default's pause function
			if (Input::GetKeyTap(EKeyCode::Pause))
			{
				s_bPaused = !s_bPaused;
				(s_bPaused) ? Time::GetInstance().Timer->Stop() : Time::GetInstance().Timer->Start();
			}

			// Game loop's core
			if (!s_bPaused)
			{
				// Updates the game and time's delta time
				Time::GetInstance().DeltaTimeMonitor();
				Game->OnUpdate();

				// Clears backbuffer and draw everything in the game
				Graphics::GetInstance().ClearBuffers();
				Game->OnDraw();
				// TODO: Renderer::Render()

				// Swaps window's framebuffers
				Graphics::GetInstance().SwapBuffers();
			}

			// Pauses the game
			else
			{
				Game->OnPause();
			}
		}

	} while (Messages.message != WM_QUIT);

	// Finalizes the game and D3D11
	Game->OnFinish();
	Graphics::GetInstance().Finalize();

	// Returns a "state code" to the OS
	return static_cast<int>(Messages.wParam);
}

void owl::Application::Pause()
{
	// Pauses the engine and time's timer
	s_bPaused = true;
	Time::GetInstance().Timer->Stop();
}

void owl::Application::Resume()
{
	// Resumes the engine and time's timer
	s_bPaused = false;
	Time::GetInstance().Timer->Start();
}
