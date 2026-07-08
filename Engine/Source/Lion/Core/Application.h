#pragma once

#ifndef LN_SHIPPING
	int main(int argc, const char* argv[]);
#else
	int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ int cmdShow);
#endif 

namespace Lion
{
	class Asset;
	class Event;
	class Layer;
	class Stack;

	class Application
	{
	public:
		LION_API Application();
		LION_API virtual ~Application();

		LION_API void PushLayer(Layer* layer);
		LION_API void PushOverlay(Layer* overlay);

		LION_API void OnEvent(Event& event);

	private:
		Scope<Asset> mAsset;
		Scope<Stack> mStack;
		bool mMinimized;

		LION_API void Run();

		void Initialize();

#ifndef LN_SHIPPING
		friend int ::main(int argc, const char* argv[]);
#else
		friend int WINAPI::WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ INT cmdShow);
#endif 
	};

	// To be defined in client
	Application* Main();
}
