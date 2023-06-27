#pragma once

namespace owl
{
	class Application
	{
	public:
		// === MAIN methods ======

		// Runs a game
		static int OWL_API Run(class Game* Level) { return GetInstance().IRun(Level); }

	protected:
		Application();
		~Application();

		// Deletes copy constructor and assigment operator
		Application(const Application&) = delete;
		Application operator=(const Application&) = delete;

		// Gets the class's static reference
		static Application& GetInstance() { static Application s_Instance; return s_Instance; }

		// === Friends ======

		friend class Input;

	private:
		// Attributes
		class Game* Game;

		// Internal MAIN methods
		int IRun(class Game* Level);

		// MAIN methods
		int Loop();

		// Static attributes
		static bool s_bPaused;

		// Static methods
		static void Pause();
		static void Resume();
	};
}