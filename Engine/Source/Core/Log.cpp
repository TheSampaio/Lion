#include "Engine.h"
#include "Log.h"

namespace Lion
{
	Log* Log::sInstance = nullptr;

	void Log::New()
	{
		sInstance = new Log();
	}

	void Log::Delete()
	{
		delete sInstance;
		sInstance = nullptr;
	}

	void Log::Console(LogLevel mode, const std::string& message)
	{
#ifndef LN_SHIPPING
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
