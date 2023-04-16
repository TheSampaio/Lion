#ifndef OWL_TIMER_H
#define OWL_TIMER_H

class Timer
{
public:
	Timer();

	// Delete copy constructor and assignment operator
	Timer(const Timer&) = delete;
	Timer operator=(const Timer&) = delete;

	// Main methods
	void Start();
	void Stop();
	float Reset();
	float GetElapsedTime();

private:
	// Attributes
	std::chrono::steady_clock::time_point m_Start;
	std::chrono::steady_clock::time_point m_End;

	bool m_bStopped;
};

#endif // !OWL_TIMER_H
