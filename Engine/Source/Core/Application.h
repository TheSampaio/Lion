#pragma once

#include "Stack.h"

int main(int argc, const char* argv[]);

namespace Lion
{
	class Layer;
	class Stack;

	class LION_API Application
	{
	public:
		Application();
		virtual ~Application();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

	private:
		Stack m_Stack;

		void Run();

		friend int ::main(int argc, const char* argv[]);
	};

	// To be defined in client.
	Application* Main();
}
