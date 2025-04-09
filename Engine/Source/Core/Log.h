#pragma once

#define LN_LOG_FORMAT(message, ...) std::format(message, __VA_ARGS__)

namespace Lion
{
	class Application;

	enum class ELogMode
	{
		Error = 0,
		Information,
		Success,
		Trace,
		Warning,
	};

	class Log
	{
	public:
		static LION_API void Console(ELogMode mode, const std::string& message);

		friend Application;

	protected:
		static Log* sInstance;

		static void New();
		static void Delete();

		Log(const Log&) = delete;
		Log operator=(const Log&) = delete;

	private:
		Log();
	};
}

