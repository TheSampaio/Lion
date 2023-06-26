#include "Sandbox.h"
#include "Resources.h"

using namespace owl;

Sandbox::Sandbox()
{
	// Set-up the window
	Window::SetIcon(IDI_ICON);
	Window::SetSize(1360, 768);
	Window::SetTitle("Sandbox");
	Window::SetDisplayMode(Windowed);
	Window::SetBackgroudColour(100, 0, 150);

	// Set-up the window's cursor
	Cursor::SetCursor(IDC_CURSOR);

	// Set-up graphics
	Graphics::SetVerticalSynchronization(Full);
}

void Sandbox::OnStart()
{
	Debug::Console(Information, "The game was started.");
}

void Sandbox::OnUpdate()
{
	if (Input::GetKeyTap(F5)) Window::Close();
}

void Sandbox::OnDraw()
{
}

void Sandbox::OnFinish()
{
	Debug::Console(Information, "The game was finished.");
}
