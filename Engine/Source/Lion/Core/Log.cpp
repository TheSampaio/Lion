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

	const std::vector<LogEntry>& Log::GetHistory()
	{
		// The Log singleton is not created in Shipping (logging is stripped), so tolerate a null
		// instance and return an empty, static history.
		static const std::vector<LogEntry> empty;
		return sInstance ? sInstance->mHistory : empty;
	}

	void Log::ClearHistory()
	{
		if (sInstance)
			sInstance->mHistory.clear();
	}

	void Log::Console(LogLevel mode, const std::string& message)
	{
#ifndef LN_SHIPPING
		// Retain every line in memory so the editor Console can display the full history, regardless
		// of which severities are routed to spdlog in the current configuration.
		if (sInstance)
		{
			if (sInstance->mHistory.size() >= kMaxHistory)
				sInstance->mHistory.erase(sInstance->mHistory.begin());

			sInstance->mHistory.push_back({ mode, CurrentTimeString(), message });
		}

		switch (mode)
		{
		case LogLevel::Error:
			spdlog::error(message);
			break;

		case LogLevel::Fatal:
			spdlog::critical(message);
			
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

	#ifndef LN_RELEASE
		case LogLevel::Information:
			spdlog::debug(message);
			break;

		case LogLevel::Trace:
			spdlog::trace(message);
			break;

	#endif // !LN_RELEASE
		}

#endif // !LN_SHIPPING
	}

	Log::Log()
	{
#ifndef LN_SHIPPING
		spdlog::set_pattern("%^[%T] %v%$");

	#ifndef LN_RELEASE
		spdlog::set_level(spdlog::level::trace);

	#endif // !LN_RELEASE

#endif // !LN_SHIPPING
	}
}
