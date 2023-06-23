#pragma once

namespace owl
{
	class OWL_API Application
	{
	public:
		static int Run(class Game* Level) { return GetInstance().IRun(Level); }

	protected:
		Application();
		~Application();

		// Deletes copy constructor and assigment operator
		Application(const Application&) = delete;
		Application operator=(const Application&) = delete;

		// Gets the class's static reference
		static Application& GetInstance() { static Application s_Instance; return s_Instance; }

	private:
		// Attributes
		class Game* Game;

		// === Internal MAIN methods ======

		int IRun(class Game* Level);
	};
}