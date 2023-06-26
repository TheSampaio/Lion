#include "Sandbox.h"

#pragma region Entry Point
#pragma warning (disable : 4251)

#ifdef WL_DEBUG

int main()
{
	return owl::Application::Run(new Sandbox);
}

#endif // !WL_DEBUG

#ifdef WL_RELEASE
#pragma comment(linker,"/SUBSYSTEM:Windows")

int WINAPI WinMain(_In_ HINSTANCE hInsance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	return owl::Application::Run(new Sandbox);
}

#endif // !WL_RELEASE

#pragma endregion
