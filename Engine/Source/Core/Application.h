#pragma once

#include "Window.h"

#ifndef LN_SHIPPING
int main(int argc, const char* argv[]);
#else
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ int cmdShow);
#endif 

namespace Lion
{
	class Layer;
	class Stack;
	class Event;

	class LION_API Application
	{
	public:
		Application();
		virtual ~Application();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		void OnEvent(Event& event);

	private:
		Stack* mStack;
		bool mMinimized;

		void Run();

#ifndef LN_SHIPPING
		friend int ::main(int argc, const char* argv[]);
#else
		friend int WINAPI::WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ int cmdShow);
#endif 
	};

	// To be defined in client
	Application* Main();
}

