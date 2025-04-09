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

	void Log::Console(ELogMode mode, const std::string& message)
	{
#ifndef LN_SHIPPING

		switch (mode)
		{
		case ELogMode::Error:
			spdlog::error(message);
			break;

		case ELogMode::Success:
			spdlog::info(message);
			break;

		case ELogMode::Warning:
			spdlog::warn(message);
			break;

	#ifndef LN_RELEASE

		case ELogMode::Information:
			spdlog::debug(message);
			break;

		case ELogMode::Trace:
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
