#pragma once

#include <Lion/Base/Platform.h>
#include <Lion/Core/Application.h>
#include <Lion/Core/ImGuiLayer.h>

namespace Lion
{
	// Dear ImGui stores its context and allocator as per-module globals. Since the engine is a
	// DLL and the client links ImGui too, the client must adopt the engine's context/allocator so
	// both sides share a single instance. Done here so client code just calls ImGui:: normally.
	inline void AdoptEngineImGui()
	{
		ImGui::SetCurrentContext(Lion::GetImGuiContext());

		ImGuiMemAllocFunc allocFunc = nullptr;
		ImGuiMemFreeFunc freeFunc = nullptr;
		void* userData = nullptr;

		Lion::GetImGuiAllocatorFunctions(&allocFunc, &freeFunc, &userData);
		ImGui::SetAllocatorFunctions(allocFunc, freeFunc, userData);
	}
}

#ifdef LN_PLATFORM_WIN
	extern Lion::Application* Lion::Main();

	#ifndef LN_SHIPPING
		int main(int argc, const char* argv[])
		{
			SetConsoleTitle(L"Lion Engine");
			auto application = Lion::Main();
			Lion::AdoptEngineImGui();
			application->Run();
			delete application;
		}

	#else
		int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ int cmdShow)
		{
			auto application = Lion::Main();
			Lion::AdoptEngineImGui();
			application->Run();
			delete application;
			return 0;
		}

	#endif // !LN_SHIPPING

#endif // !LN_PLATFORM_WIN
