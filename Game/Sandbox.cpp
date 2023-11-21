#include "Sandbox.h"
#include "Data/Resources.h"

using namespace Lion;

// Initializes the scene's pointer
Scene* Sandbox::s_Scene = nullptr;

Sandbox::Sandbox()
{
	// Game's initial's setup
	{
		// Set-up the window
		Window::SetIcon(IDI_ICON);
		Window::SetSize(800, 600);
		Window::SetTitle("Galaga Demo");
		//Window::SetDisplayMode(Windowed);

		// Set-up the window's cursor
		Cursor::SetCursor(IDC_CURSOR);
	}

	// Initializes all pointers
	m_Background = nullptr;
	m_Header = nullptr;
	m_Player = nullptr;
	m_Alien = nullptr;

	m_Alien01 = nullptr;
	m_Alien02 = nullptr;
	m_Alien03 = nullptr;
	m_Alien04 = nullptr;
}

void Sandbox::OnStart()
{
	Debug::Console(Information, "The game was initialized.");

	// Initializes the scene and background
	s_Scene = new Scene();
	m_Background = new Sprite("Data/Galaga/background.png");

	m_Header = new Sprite("Data/Galaga/header.png");

	m_Player = new Spaceship();
	s_Scene->Add(m_Player);

	// Loads all alien's textures
	m_Alien01 = new Texture("Data/Galaga/alien-01.png");
	m_Alien02 = new Texture("Data/Galaga/alien-02.png");
	m_Alien03 = new Texture("Data/Galaga/alien-03.png");
	m_Alien04 = new Texture("Data/Galaga/alien-04.png");

	float X = 0.0f, Y = 280.0f, Offset = 64.0f;

	// First row
	s_Scene->Add(new Alien(m_Alien01, X, Y));
	s_Scene->Add(new Alien(m_Alien02, X + Offset, Y));
	s_Scene->Add(new Alien(m_Alien03, X + Offset * 2, Y));
	s_Scene->Add(new Alien(m_Alien04, X + Offset * 3, Y));
	s_Scene->Add(new Alien(m_Alien03, X + Offset * 4, Y));
	s_Scene->Add(new Alien(m_Alien02, X + Offset * 5, Y));
	s_Scene->Add(new Alien(m_Alien01, X + Offset * 6, Y));
	s_Scene->Add(new Alien(m_Alien02, X + Offset * 7, Y));
	s_Scene->Add(new Alien(m_Alien03, X + Offset * 8, Y));
	s_Scene->Add(new Alien(m_Alien04, X + Offset * 9, Y));
	s_Scene->Add(new Alien(m_Alien03, X + Offset * 10, Y));
	s_Scene->Add(new Alien(m_Alien02, X + Offset * 11, Y));
	s_Scene->Add(new Alien(m_Alien04, X + Offset * 12, Y));

	X = Window::GetSize()[0] - 62.0f;
	Y = 330.0f;

	// Second row
	s_Scene->Add(new Alien(m_Alien04, X, Y));
	s_Scene->Add(new Alien(m_Alien03, X - Offset, Y));
	s_Scene->Add(new Alien(m_Alien02, X - Offset * 2, Y));
	s_Scene->Add(new Alien(m_Alien01, X - Offset * 3, Y));
	s_Scene->Add(new Alien(m_Alien02, X - Offset * 4, Y));
	s_Scene->Add(new Alien(m_Alien03, X - Offset * 5, Y));
	s_Scene->Add(new Alien(m_Alien04, X - Offset * 6, Y));
	s_Scene->Add(new Alien(m_Alien03, X - Offset * 7, Y));
	s_Scene->Add(new Alien(m_Alien02, X - Offset * 8, Y));
	s_Scene->Add(new Alien(m_Alien01, X - Offset * 9, Y));
	s_Scene->Add(new Alien(m_Alien02, X - Offset * 10, Y));
	s_Scene->Add(new Alien(m_Alien03, X - Offset * 11, Y));
	s_Scene->Add(new Alien(m_Alien01, X - Offset * 12, Y));
}

void Sandbox::OnUpdate()
{
	// Updates entities
	s_Scene->Update();
}

void Sandbox::OnDraw()
{
	// Draws entities
	s_Scene->Draw();

	// Draws the background
	m_Background->Draw(0.0f, 0.0f, Layer::Back);

	// Draws the game's header
	m_Header->Draw(static_cast<float>(Window::GetCenter()[0] - m_Header->GetCenter()[0]), 40.0f, Layer::Front);
}

void Sandbox::OnFinish()
{
	// Deletes the scene with all entities inside
	delete s_Scene;

	// Deletes the background
	delete m_Background;

	// Deletes the header
	delete m_Header;

	// Deletes all textures
	delete m_Alien01;
	delete m_Alien02;
	delete m_Alien03;
	delete m_Alien04;

	Debug::Console(Information, "The game was finalized.");
}

#pragma region Entry Point

#ifdef LN_DEBUG

int main()
{
	return Lion::Application::Run(new Sandbox);
}

#endif // !LN_DEBUG

#ifdef LN_RELEASE

#pragma comment(linker,"/SUBSYSTEM:Windows")

int WINAPI WinMain(_In_ HINSTANCE hInsance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nShowCmd)
{
	return Lion::Application::Run(new Sandbox);
}

#endif // !LN_RELEASE

#pragma endregion
