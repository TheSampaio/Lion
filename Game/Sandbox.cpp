#include "Sandbox.h"
#include "Resources.h"

using namespace owl;

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
		Window::SetDisplayMode(Windowed);

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

	float X = 10.0f, Y = 280.0f;

	// First row
	s_Scene->Add(new Alien(m_Alien01, X, Y));
	s_Scene->Add(new Alien(m_Alien02, X + 100, Y));
	s_Scene->Add(new Alien(m_Alien03, X + 200, Y));
	s_Scene->Add(new Alien(m_Alien04, X + 300, Y));
	s_Scene->Add(new Alien(m_Alien01, X + 400, Y));
	s_Scene->Add(new Alien(m_Alien02, X + 500, Y));
	s_Scene->Add(new Alien(m_Alien03, X + 600, Y));
	s_Scene->Add(new Alien(m_Alien04, X + 700, Y));

	X = 20.0f;
	float OffsetX = 44.0f;
	float OffsetY = 60.0f;

	// Second row
	s_Scene->Add(new Alien(m_Alien04, X       + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien03, X + 100 + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien02, X + 200 + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien01, X + 300 + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien04, X + 400 + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien03, X + 500 + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien02, X + 600 + OffsetX, Y + OffsetY));
	s_Scene->Add(new Alien(m_Alien01, X + 700 + OffsetX, Y + OffsetY));
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
