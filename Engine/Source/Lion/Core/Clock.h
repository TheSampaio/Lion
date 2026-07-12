#pragma once

#include <Lion/Logic/Timer.h>

namespace Lion
{
	class Application;

	class Clock
	{
	public:
		static LION_API float32 GetDeltaTime() { return sFrameTime; }

		// Whether the window title carries the frame stats (FPS, frame time, build, graphics API).
		//
		// A diagnostic, so it follows the same rule as the log: a runtime switch rather than a #ifdef,
		// because the engine is a single DLL shared by the editor and the game and a compile-time one
		// would silence both at once. Defaults to what the build asks for — on, except in a shipped
		// game — and the editor turns it on whatever build it was compiled in.
		static LION_API void SetShowFrameStats(bool show);
		static LION_API bool GetShowFrameStats();

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
		static uint32 sFrameCount;
		static float32 sTotalTime;

		static Timer& GetTimer() { return *sInstance->mTimer; }

		static void UpdateFrameTime();
	};
}
