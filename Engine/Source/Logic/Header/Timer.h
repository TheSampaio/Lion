#pragma once

namespace owl
{
	class OWL_API Timer
	{
	public:
		Timer();

		// === MAIN methods ======

		// Starts the timer
		void Start();

		// Stops the timer
		void Stop();

		// Resets the timer
		float Reset();

		// === GET methods ======

		// Gets the elapsed time since the timer's start
		float GetElapsedTime();

	private:
		// Attributes
		std::chrono::steady_clock::time_point m_Start;
		std::chrono::steady_clock::time_point m_End;

		bool m_bPaused;
	};
}