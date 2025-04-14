#pragma once

namespace Lion
{
	class Timer
    {
    public:
        Timer();

        // Gets the elapsed time since the timer's start
        float32 GetElapsedTime();

        // Starts the timer
        void Start();

        // Stops the timer
        void Stop();

        // Resets the timer
        float32 Reset();

    private:
        std::chrono::steady_clock::time_point mStart;
        std::chrono::steady_clock::time_point mEnd;
        bool mIsPaused;
    };
}
