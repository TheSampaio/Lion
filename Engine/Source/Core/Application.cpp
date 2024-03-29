#include "Engine.h"
#include "Header/Application.h"

#include "Header/Window.h"
#include "../Event/Header/Debug.h"
#include "../Event/Header/Input.h"
#include "../Event/Header/Time.h"
#include "../Logic/Header/Game.h"
#include "../Logic/Header/Timer.h"
#include "../Render/Header/Graphics.h"
#include "../Render/Header/Renderer.h"

bool Lion::Application::s_bPaused = false;

Lion::Application::Application()
	: Game(nullptr)
{
	// Changes console's title (Only in debug mode)
#ifdef LN_DEBUG
	std::system("TITLE Lion Engine");
#endif // LN_DEBUG
}

Lion::Application::~Application()
{
	// Deletes the hame
	delete Game;
}

int Lion::Application::IRun(class Game* Level)
{
	// Creates the game
	Game = Level;

	// Creates the window
	if (!Window::GetInstance().Create())
	{
		Debug::Message(Error, "Failed to create the window.");
		return false;
	}

	// Initializes the D3D11
	if (!Graphics::GetInstance().Initialize())
	{
		Debug::Message(Error, "Failed to initialize the DirectX.");
		return false;
	}

	// Initializes the renderer
	if (!Renderer::GetInstance().Initialize())
	{
		Debug::Message(Error, "Failed to initialize the renderer.");
		return false;
	}

	// Game loop
	timeBeginPeriod(1); // Changes the default "Sleep()" functions' precision
	int State = Loop(); // Runs the engine's game loop
	timeBeginPeriod(1); // Restores the default "Sleep()" functions' precision

	// Returns game loop's state
	return State;
}

int Lion::Application::Loop()
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
			if (Input::GetKeyTap(EKeyCode::Key_Pause))
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

				Renderer::GetInstance().Render();

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

	// Finalizes the game
	Game->OnFinish();

	// Finalizes the D3D11 and renderer
	Graphics::GetInstance().Finalize();
	Renderer::GetInstance().Finalize();

	// Returns a "state code" to the OS
	return static_cast<int>(Messages.wParam);
}

void Lion::Application::Pause()
{
	// Pauses the engine and time's timer
	s_bPaused = true;
	Time::GetInstance().Timer->Stop();
}

void Lion::Application::Resume()
{
	// Resumes the engine and time's timer
	s_bPaused = false;
	Time::GetInstance().Timer->Start();
}
