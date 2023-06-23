#include "Sandbox.h"

using namespace owl;

void Sandbox::OnStart()
{
	Debug::Console(Information, "The game was started.");
}

void Sandbox::OnUpdate()
{
#ifdef WL_DEBUG
	if (Input::GetKeyTap(F5)) Window::Close();
#endif // WL_DEBUG
}

void Sandbox::OnDraw()
{
}

void Sandbox::OnFinish()
{
	Debug::Console(Information, "The game was finished.");
}
