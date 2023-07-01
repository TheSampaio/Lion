#include "Source/Sandbox.h"

#pragma region Entry Point

#ifdef WL_DEBUG

int main()
{
	return owl::Application::Run(new Sandbox);
}

#endif // !WL_DEBUG

#ifdef WL_RELEASE
#pragma comment(linker,"/SUBSYSTEM:Windows")

int WINAPI WinMain(_In_ HINSTANCE hInsance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nShowCmd)
{
	return owl::Application::Run(new Sandbox);
}

#endif // !WL_RELEASE

#pragma endregion
