#pragma once

#include <Lion/Base/Platform.h>
#include <Lion/Core/Application.h>
#include <Lion/Core/CommandLine.h>

#ifdef LN_PLATFORM_WIN
	extern Lion::Application* Lion::Main();

	#ifndef LN_SHIPPING
		int main(int argc, const char* argv[])
		{
			SetConsoleTitle(L"Lion Engine");

			// Before the application is built, so a layer can read them from OnCreate. Windows opens a
			// scene by running the editor and handing it the path, and an editor that cannot read its own
			// arguments would take the double-click and show an empty scene.
			Lion::CommandLine::Set(argc, argv);

			auto application = Lion::Main();
			application->Run();
			delete application;
		}

	#else
		int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ int cmdShow)
		{
			// A windowed entry point is handed one string; the CRT has already split it, which is what
			// __argv is, and splitting it a second time by hand would be a second answer to one question.
			Lion::CommandLine::Set(__argc, const_cast<const char**>(__argv));

			auto application = Lion::Main();
			application->Run();
			delete application;
			return 0;
		}

	#endif // !LN_SHIPPING

#endif // !LN_PLATFORM_WIN
