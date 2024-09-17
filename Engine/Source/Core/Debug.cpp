#include "Engine.h"
#include "Debug.h"

namespace Lion
{
	Debug* Debug::sInstance = nullptr;

	void Debug::New()
	{
		sInstance = new Debug();
	}

	Debug& Debug::Get()
	{
		return *sInstance;
	}

	void Debug::Delete()
	{
		delete sInstance;
		sInstance = nullptr;
	}

	void Debug::Console(EDebugMode mode, const std::string& message)
	{
#ifndef LN_DISTRIBUTION

		switch (mode)
		{
		case Error:
			spdlog::error(message);
			break;

		case Warning:
			spdlog::warn(message);
			break;

#ifndef LN_RELEASE

		case Information:
			spdlog::info(message);
			break;

		case Trace:
			spdlog::trace(message);
			break;

#endif // !LN_RELEASE
		}

#endif // !LN_DISTRIBUTION
	}

	Debug::Debug()
	{
#ifndef LN_DISTRIBUTION
		spdlog::set_pattern("%^[%T] %v%$");

#ifndef LN_RELEASE
		spdlog::set_level(spdlog::level::trace);

#endif // !LN_RELEASE

#endif // !LN_DISTRIBUTION
	}
}
