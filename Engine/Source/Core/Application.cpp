#include "Engine.h"
#include "Header/Application.h"

#include "Header/Window.h"
#include "../Event/Header/Debug.h"
#include "../Event/Header/Input.h"
#include "../Event/Header/Time.h"
#include "../Logic/Header/Game.h"
#include "../Logic/Header/Timer.h"

bool owl::Application::s_bPaused = false;

owl::Application::Application()
	: Game(nullptr)
{
#ifdef WL_DEBUG
	std::system("TITLE Owl Engine");
#endif // WL_DEBUG
}

owl::Application::~Application()
{
	delete Game;
}

int owl::Application::IRun(class Game* Level)
{
	Game = Level;

	// Creates the window
	if (!Window::GetInstance().Create())
	{
		Debug::Message(Error, "Failed to create the window.");
		return false;
	}

	// TODO: Creates the renderer
	// TODO: Creates the graphics

	// Game loop
	timeBeginPeriod(1);
	int State = Loop();
	timeBeginPeriod(1);

	return State;
}

int owl::Application::Loop()
{
	Time::GetInstance().Timer->Start();
	Game->OnStart();

	MSG Messages = { 0 };

	do
	{
		if (PeekMessage(&Messages, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Messages);
			DispatchMessage(&Messages);
		}

		else
		{
			if (Input::GetKeyTap(EKeyCode::Pause))
			{
				s_bPaused = !s_bPaused;
				(s_bPaused) ? Time::GetInstance().Timer->Stop() : Time::GetInstance().Timer->Start();
			}

			if (!s_bPaused)
			{
				Time::GetInstance().DeltaTimeMonitor();
				Game->OnUpdate();

				// TODO: Clears back framebuffer
				Game->OnDraw();
				// TODO: Renderer::Render()

				// TODO: Swaps framebuffers
			}

			else
			{
				Game->OnPause();
			}
		}

	} while (Messages.message != WM_QUIT);

	Game->OnFinish();

	return static_cast<int>(Messages.wParam);
}

void owl::Application::Pause()
{
	s_bPaused = true;
	Time::GetInstance().Timer->Stop();
}

void owl::Application::Resume()
{
	s_bPaused = false;
	Time::GetInstance().Timer->Start();
}
