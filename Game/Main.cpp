#include "Sandbox.h"

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
