#include "Engine.h"
#include "Header/Application.h"

#include "Header/Window.h"
#include "../Event/Header/Debug.h"
#include "../Logic/Header/Game.h"

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
	// Creates the window
	if (!Window::GetInstance().Create())
	{
		Debug::Message(Error, "Failed to create the window.");
		return false;
	}

	// TODO: Creates the renderer
	// TODO: Creates the graphics

	Game = Level;
	// TODO: Starts the timer
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
			// TODO: Timer::GetDeltaTime()
			Game->OnUpdate();

			// TODO: Clears back framebuffer
			Game->OnDraw();
			// TODO: Renderer::Render()

			// TODO: Swaps framebuffers
		}

	} while (Messages.message != WM_QUIT);

	Game->OnFinish();
	return static_cast<int>(Messages.wParam);
}
