#pragma once

namespace owl
{
	class OWL_API Time
	{
	public:
		// === GET methods ======

		static float GetDeltaTime() { return GetInstance().m_DeltaTime; }

		// === Friends ======
		friend class Application;

	protected:
		Time();
		~Time();

		// Deletes copy constructor and assigment operator
		Time(const Time&) = delete;
		Time operator=(const Time&) = delete;

		// Gets the class's static reference
		static Time& GetInstance() { static Time s_Instance; return s_Instance; }

	private:
		// Attribute
		class Timer* Timer;
		float m_DeltaTime;

		// === MAIN methods ======

		void DeltaTimeMonitor();
	};
}