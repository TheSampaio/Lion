#pragma once

namespace owl
{
	class Timer
	{
	public:
		Timer();

		// === MAIN methods ======

		// Starts the timer
		void OWL_API Start();

		// Stops the timer
		void OWL_API Stop();

		// Resets the timer
		float OWL_API Reset();

		// === GET methods ======

		// Gets the elapsed time since the timer's start
		float OWL_API GetElapsedTime();

	private:
		// Attributes
		std::chrono::steady_clock::time_point m_Start;
		std::chrono::steady_clock::time_point m_End;

		bool m_bPaused;
	};
}