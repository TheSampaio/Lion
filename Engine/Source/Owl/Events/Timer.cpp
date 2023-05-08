#include "Core.h"
#include "Include/Timer.h"

using namespace std::chrono;

Timer::Timer()
{
	// Initializes all class's attributes
	m_Start = steady_clock::time_point();
	m_End = steady_clock::time_point();

	m_bStopped = false;
}

void Timer::Start()
{
	if (m_bStopped)
	{
		// Calculates elapsed time
		duration ElapsedTime = m_End - m_Start;

		// Starts time considering the elapsed time
		m_Start = steady_clock::now();
		m_Start -= ElapsedTime;
		m_bStopped = false;
	}

	else
	{
		// Starts timer's count
		m_Start = steady_clock::now();
	}
}

void Timer::Stop()
{
	if (!m_bStopped)
	{
		// Sets timer's stop
		m_End = steady_clock::now();
		m_bStopped = true;
	}
}

float Timer::Reset()
{
	duration ElapsedTime = nanoseconds(0);

	if (m_bStopped)
	{
		// Gets elapsed time before stops
		ElapsedTime = m_End - m_Start;

		// Reset timer's count
		m_Start = steady_clock::now();
		m_bStopped = false;
	}

	else
	{
		// Stops timer's count and get the elapsed time
		m_End = steady_clock::now();
		ElapsedTime = m_End - m_Start;

		// Resets timer's count
		m_Start = m_End;
	}

	// Returns time in seconds
	return static_cast<float>(duration<double>(ElapsedTime).count());
}

float Timer::GetElapsedTime()
{
	duration ElapsedTime = nanoseconds(0);

	if (m_bStopped)
	{
		// Calculates elapsed time
		ElapsedTime = m_End - m_Start;
	}

	else
	{
		// Stops timer's count and get the elapsed time
		m_End = steady_clock::now();
		ElapsedTime = m_End - m_Start;
	}

	// Returns time in seconds
	return static_cast<float>(duration<double>(ElapsedTime).count());
}
