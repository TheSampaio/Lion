#pragma once

int main(int argc, const char* argv[]);

namespace Lion
{
	class Layer;

	class LION_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Push(Layer* layer);

	private:
		Layer* m_Layer; // Stores the pushed layer (Temporary)

		void Run();

		friend int ::main(int argc, const char* argv[]);
	};

	// To be defined in client.
	Application* Main();
}
