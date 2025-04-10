#pragma once

namespace Lion
{
	class Application;

	enum class ELogMode
	{
		Error = 0,
		Fatal,
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

