#pragma once

namespace Lion
{
	class Timer
	{
	public:
		Timer();

		// === MAIN methods ======

		// Starts the timer
		void LION_API Start();

		// Stops the timer
		void LION_API Stop();

		// Resets the timer
		float LION_API Reset();

		// === GET methods ======

		// Gets the elapsed time since the timer's start
		float LION_API GetElapsedTime();

	private:
		// Attributes
		std::chrono::steady_clock::time_point m_Start;
		std::chrono::steady_clock::time_point m_End;

		bool m_bPaused;
	};
}