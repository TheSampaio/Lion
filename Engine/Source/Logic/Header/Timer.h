#pragma once

namespace owl
{
	class OWL_API Timer
	{
	public:
		Timer();

		// === MAIN methods ======

		void Start();
		void Stop();
		float Reset();

		// === GET methods ======

		float GetElapsedTime();

	private:
		// Attributes
		std::chrono::steady_clock::time_point m_Start;
		std::chrono::steady_clock::time_point m_End;

		bool m_bPaused;
	};
}