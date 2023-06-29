#include "Sandbox.h"
#include "Resources.h"

using namespace owl;

Sandbox::Sandbox()
{
	// Game main's setup
	{
		// Set-up the window
		Window::SetIcon(IDI_ICON);
		Window::SetSize(800, 600);
		Window::SetTitle("Sandbox");
		Window::SetDisplayMode(Windowed);
		Window::SetBackgroudColour(100, 0, 150);

		// Set-up the window's cursor
		Cursor::SetCursor(IDC_CURSOR);

		// Set-up graphics
		Graphics::SetVerticalSynchronization(Full);
	}

	m_HeaderTexture = nullptr;
	m_Header01 = nullptr;
	m_Header02 = nullptr;

	m_Background = nullptr;
	m_Player = nullptr;

	m_Speed = 0.0f;
}

void Sandbox::OnStart()
{
	Debug::Console(Information, "The game was started.");

	m_HeaderTexture = new Texture("Resource/Sprites/Logo.png");
	m_Header01 = new Sprite(m_HeaderTexture);
	m_Header02 = new Sprite(m_HeaderTexture);

	m_Background = new Sprite("Resource/Sprites/background.jpg");
	m_Player = new Sprite("Resource/Sprites/shank.png");
}

void Sandbox::OnUpdate()
{
	m_Speed += 100.0f * Time::GetDeltaTime();

	if (m_Speed > Window::GetSize()[0])
	{
		m_Speed = static_cast<float>(-static_cast<int>(m_Player->GetSize()[0]));
	}
}

void Sandbox::OnDraw()
{
	m_Header01->Draw(40.0f, 60.0f, Layer::Upper);
	m_Header02->Draw(400.0f, 450.0f, Layer::Lower);

	m_Background->Draw(0.0f, 0.0f, Layer::Back);
	m_Player->Draw(m_Speed, 90.0f, Layer::Middle);
}

void Sandbox::OnFinish()
{
	delete m_HeaderTexture;
	delete m_Header01;
	delete m_Header02;
	delete m_Player;
	delete m_Background;

	Debug::Console(Information, "The game was finished.");
}
