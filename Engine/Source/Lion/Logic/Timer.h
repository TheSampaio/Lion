#pragma once

#include <Lion/Base/Platform.h>

namespace Lion
{
	class Timer
    {
    public:
        Timer();

        // Gets the elapsed time since the timer's start
        LION_API float32 GetElapsedTime();

        // Starts the timer
        LION_API void Start();

        // Stops the timer
        LION_API void Stop();

        // Resets the timer
        LION_API float32 Reset();

    private:
        std::chrono::steady_clock::time_point mStart;
        std::chrono::steady_clock::time_point mEnd;
        bool mIsPaused;
    };
}
