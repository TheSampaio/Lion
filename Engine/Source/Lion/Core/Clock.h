#pragma once

#include <Lion/Logic/Timer.h>

namespace Lion
{
	class Application;

	class Clock
	{
	public:
		static LION_API float32 GetDeltaTime() { return sFrameTime; }

		friend Application;

	protected:
		static Clock* sInstance;

		static void New();
		static void Delete();

		Clock(const Clock&) = delete;
		Clock& operator=(const Clock&) = delete;

	private:
		Scope<Timer> mTimer;

		Clock();

		static float32 sFrameTime;

#ifndef LN_SHIPPING
		static uint32 sFrameCount;
		static float32 sTotalTime;

#endif // LN_SHIPPING

		static Timer& GetTimer() { return *sInstance->mTimer; }

		static void UpdateFrameTime();
	};
}
