#pragma once

#ifdef LN_PLATFORM_WIN
	extern Lion::Application* Lion::Main();

	#ifndef LN_SHIPPING
		int main(int argc, const char* argv[])
		{
			SetConsoleTitle(L"Lion Engine");
			auto application = Lion::Main();
			application->Run();
			delete application;
		}

	#else
		#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

		int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ int cmdShow)
		{
			auto application = Lion::Main();
			application->Run();
			delete application;
			return 0;
		}

	#endif // !LN_SHIPPING

#endif // !LN_PLATFORM_WIN
