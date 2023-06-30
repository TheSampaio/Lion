#include "Sandbox.h"
#include "Resources.h"

#include "Shank.h"
#include "Header.h"

using namespace owl;

Sandbox::Sandbox()
{
	{ // Game's initial's setup

		// Set-up the window
		Window::SetIcon(IDI_ICON);
		Window::SetSize(800, 600);
		Window::SetTitle("Shank Demo");
		Window::SetDisplayMode(Windowed);
		//Window::SetBackgroudColour(100, 0, 150);

		// Set-up the window's cursor
		Cursor::SetCursor(IDC_CURSOR);

		// Set-up graphics
		//Graphics::SetVerticalSynchronization(Full);
	}

	m_HeaderTexture = nullptr;
	m_Header01 = nullptr;
	m_Header02 = nullptr;
	
	m_Player = nullptr;
	m_Background = nullptr;
}

void Sandbox::OnStart()
{
	m_HeaderTexture = new Texture("Resource/Shank/Sprite/header.png");

	m_Header01 = new Header(m_HeaderTexture);
	m_Header01->SetPosition(40.0f, 60.0f, Layer::Upper);

	m_Header02 = new Header(m_HeaderTexture);
	m_Header02->SetPosition(400.0f, 450.0f, Layer::Lower);

	m_Player = new Shank();
	m_Background = new Sprite("Resource/Shank/Sprite/background.jpg");
}

void Sandbox::OnUpdate()
{
	m_Player->OnUpdate();
}

void Sandbox::OnDraw()
{
	m_Header01->OnDraw();
	m_Header02->OnDraw();

	m_Player->OnDraw();
	m_Background->Draw(0, 0, Layer::Back);
}

void Sandbox::OnFinish()
{
	delete m_Background;
	delete m_Player;

	delete m_Header02;
	delete m_Header01;
	delete m_HeaderTexture;
}
