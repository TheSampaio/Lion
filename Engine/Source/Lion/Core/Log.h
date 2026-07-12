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

	// How much of the log gets through, in increasing order.
	//
	// This is a runtime setting rather than a compile-time one, and it has to be: the engine is a
	// single DLL shared by the editor and the game, so a #ifdef would silence both at once. The editor
	// turns everything on whatever build it was compiled in — a tool that cannot say what happened is
	// not a tool — while a game keeps its build's default, which for an exported one is nothing.
	enum class LogVerbosity
	{
		None,      // Nothing at all.
		Errors,    // Error and Fatal.
		Normal,    // ...and Warning and Success.
		Verbose,   // ...and Information and Trace. Everything.
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

		// Defaults to what the build asks for — everything in Debug, warnings and successes in Release,
		// nothing in Shipping. An application overrides it at start-up; the editor asks for everything.
		static LION_API void SetVerbosity(LogVerbosity verbosity);
		static LION_API LogVerbosity GetVerbosity();

		// Whether a level is currently let through, so a caller can skip building a message that would
		// only be thrown away.
		static LION_API bool IsEnabled(LogLevel level);

		// In-memory history of every logged line (oldest first), for editor/tooling consumption.
		// A deque so dropping the oldest entry at the cap is O(1) instead of shifting the whole
		// buffer, while still allowing the editor to index into it.
		static LION_API const std::deque<LogEntry>& GetHistory();

		// Number of lines logged since startup. Unlike the history size it never decreases, so tools
		// can tell that new lines arrived even once the history saturates at its cap.
		static LION_API size_t GetTotalCount();

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

		std::deque<LogEntry> mHistory;
		size_t mTotalCount = 0;
	};
}
