#include "Engine.h"
#include "Log.h"

#include <ctime>

namespace Lion
{
	Log* Log::sInstance = nullptr;

	namespace
	{
		// Current local time-of-day as "HH:MM:SS", matching the spdlog console pattern.
		std::string CurrentTimeString()
		{
			const std::time_t now = std::time(nullptr);
			std::tm local{};

#if LN_PLATFORM_WIN
			localtime_s(&local, &now);
#else
			localtime_r(&now, &local);
#endif

			char buffer[16];
			std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);
			return buffer;
		}

		// Kept outside the Log instance, so an application can set it before the engine starts and even
		// the start-up messages obey it.
		LogVerbosity& Verbosity()
		{
			static LogVerbosity verbosity =
#if defined(LN_DEBUG)
				LogVerbosity::Verbose;
#elif defined(LN_RELEASE)
				LogVerbosity::Normal;
#else
				LogVerbosity::None;
#endif

			return verbosity;
		}

		// The lowest verbosity that still lets this level through.
		LogVerbosity RequiredVerbosity(LogLevel level)
		{
			switch (level)
			{
				case LogLevel::Error:
				case LogLevel::Fatal:       return LogVerbosity::Errors;

				case LogLevel::Warning:
				case LogLevel::Success:     return LogVerbosity::Normal;

				case LogLevel::Information:
				case LogLevel::Trace:       return LogVerbosity::Verbose;
			}

			return LogVerbosity::Verbose;
		}
	}

	void Log::SetVerbosity(LogVerbosity verbosity)
	{
		Verbosity() = verbosity;
	}

	LogVerbosity Log::GetVerbosity()
	{
		return Verbosity();
	}

	bool Log::IsEnabled(LogLevel level)
	{
		return static_cast<int32>(Verbosity()) >= static_cast<int32>(RequiredVerbosity(level));
	}

	void Log::New()
	{
		sInstance = new Log();
	}

	void Log::Delete()
	{
		delete sInstance;
		sInstance = nullptr;
	}

	const std::deque<LogEntry>& Log::GetHistory()
	{
		// The Log singleton is not created in Shipping (logging is stripped), so tolerate a null
		// instance and return an empty, static history.
		static const std::deque<LogEntry> empty;
		return sInstance ? sInstance->mHistory : empty;
	}

	size_t Log::GetTotalCount()
	{
		return sInstance ? sInstance->mTotalCount : 0;
	}

	void Log::ClearHistory()
	{
		if (sInstance)
			sInstance->mHistory.clear();
	}

	void Log::Console(LogLevel mode, const std::string& message)
	{
		// The verbosity is the only filter. spdlog is left wide open below, so the two cannot disagree
		// about what was meant to be logged.
		if (!IsEnabled(mode))
			return;

		// Retain every line in memory so the editor's Console panel can display the full history.
		if (sInstance)
		{
			if (sInstance->mHistory.size() >= kMaxHistory)
				sInstance->mHistory.pop_front();

			sInstance->mHistory.push_back({ mode, CurrentTimeString(), message });
			++sInstance->mTotalCount;
		}

		switch (mode)
		{
		case LogLevel::Error:
			spdlog::error(message);
			break;

		case LogLevel::Fatal:
			spdlog::critical(message);

			// Halting on a fatal is a development aid: it only makes sense while someone is watching
			// the log, which is exactly what the verbosity says.
#if LN_PLATFORM_WIN
			system("PAUSE");

#else
			std::puts("Press Enter to continue . . .");
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

#endif

			break;

		case LogLevel::Success:
			spdlog::info(message);
			break;

		case LogLevel::Warning:
			spdlog::warn(message);
			break;

		case LogLevel::Information:
			spdlog::debug(message);
			break;

		case LogLevel::Trace:
			spdlog::trace(message);
			break;
		}
	}

	Log::Log()
	{
		spdlog::set_pattern("%^[%T] %v%$");

		// Wide open on purpose: the verbosity above is the single filter, and a second one here could
		// only drift from it.
		spdlog::set_level(spdlog::level::trace);
	}
}
