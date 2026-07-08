#pragma once

namespace Lion
{
	class Application;

	enum class LogLevel
	{
		Error = 0,
		Fatal,
		Information,
		Success,
		Trace,
		Warning,
	};

	// A single captured log line, retained in memory so tools (e.g. the editor Console panel) can
	// display the engine's and game's log output.
	struct LogEntry
	{
		LogLevel level;
		std::string time;     // Local time-of-day the entry was recorded, formatted as "HH:MM:SS".
		std::string message;
	};

	class Log
	{
	public:
		static LION_API void Console(LogLevel mode, const std::string& message);

		// In-memory history of every logged line (oldest first), for editor/tooling consumption.
		static LION_API const std::vector<LogEntry>& GetHistory();

		// Clears the in-memory log history (does not affect already-printed console output).
		static LION_API void ClearHistory();

		friend Application;

	protected:
		static Log* sInstance;

		static void New();
		static void Delete();

		Log(const Log&) = delete;
		Log operator=(const Log&) = delete;

	private:
		Log();

		// Maximum number of retained entries; the oldest are dropped once the cap is reached.
		static constexpr size_t kMaxHistory = 1024;

		std::vector<LogEntry> mHistory;
	};
}
