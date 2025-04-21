#include "Engine.h"
#include "Timer.h"

using namespace std::chrono;

namespace Lion
{
	Timer::Timer()
	{
		// Initializes all class's attributes
		mStart = steady_clock::time_point();
		mEnd = steady_clock::time_point();

		mIsPaused = false;
	}

	float32 Timer::GetElapsedTime()
	{
		duration ElapsedTime = nanoseconds(0);

		if (mIsPaused)
		{
			// Calculates elapsed time
			ElapsedTime = mEnd - mStart;
		}

		else
		{
			// Stops timer's count and get the elapsed time
			mEnd = steady_clock::now();
			ElapsedTime = mEnd - mStart;
		}

		// Returns time in seconds
		return static_cast<float32>(duration<float64>(ElapsedTime).count());
	}

	void Timer::Start()
	{
		if (mIsPaused)
		{
			// Calculates elapsed time
			duration ElapsedTime = mEnd - mStart;

			// Starts time considering the elapsed time
			mStart = steady_clock::now();
			mStart -= ElapsedTime;
			mIsPaused = false;
		}

		else
		{
			// Starts timer's count
			mStart = steady_clock::now();
		}
	}

	void Timer::Stop()
	{
		if (!mIsPaused)
		{
			// Sets timer's stop
			mEnd = steady_clock::now();
			mIsPaused = true;
		}
	}

	float32 Timer::Reset()
	{
		duration ElapsedTime = nanoseconds(0);

		if (mIsPaused)
		{
			// Gets elapsed time before stops
			ElapsedTime = mEnd - mStart;

			// Reset timer's count
			mStart = steady_clock::now();
			mIsPaused = false;
		}

		else
		{
			// Stops timer's count and get the elapsed time
			mEnd = steady_clock::now();
			ElapsedTime = mEnd - mStart;

			// Resets timer's count
			mStart = mEnd;
		}

		// Returns time in seconds
		return static_cast<float32>(duration<float64>(ElapsedTime).count());
	}
}
