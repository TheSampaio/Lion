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

		// One turn of the loop: update, render, present.
		//
		// It is also what the window calls back into while the user is dragging its edge. Windows runs a
		// loop of its own for that and does not give the application its thread back until the drag ends,
		// so a frame drawn only from Run() is a frame that is not drawn at all for as long as the resize
		// lasts — which is why the window went white.
		void Frame();

		bool mInFrame = false;   // A frame drawn from inside a frame would be two frames at once.

#ifndef LN_SHIPPING
		friend int ::main(int argc, const char* argv[]);
#else
		friend int WINAPI::WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hInstancePrev, _In_ PSTR cmdLine, _In_ INT cmdShow);
#endif 
	};

	// To be defined in client
	Application* Main();
}
