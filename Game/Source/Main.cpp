#pragma warning (disable : 4251)

#include "Sandbox.h"
#include "Resources.h"

using namespace owl;

int Main()
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

	// Runs the game
	return Application::Run(new Sandbox);
}

#pragma region Entry Point
#ifdef WL_DEBUG
int main()
{
	return Main();
}
#endif // !WL_DEBUG
#ifdef WL_RELEASE
#pragma comment(linker,"/SUBSYSTEM:Windows")
int WINAPI WinMain(_In_ HINSTANCE hInsance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	return Main();
}
#endif // !WL_RELEASE
#pragma endregion
