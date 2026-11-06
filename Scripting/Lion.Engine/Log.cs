using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Writes to the same verbosity-filtered log and editor Console as native gameplay.</summary>
/// <remarks>Engine-thread-only. Check an enabled property before constructing expensive messages.</remarks>
public static class Log
{
	/// <summary>Reports trace diagnostics when the native verbosity permits them.</summary>
	public static void Trace(string message) => NativeApi.WriteLog(4, message);
	/// <summary>Reports success when the native verbosity permits it.</summary>
	public static void Success(string message) => NativeApi.WriteLog(3, message);
	/// <summary>Whether an informational message would be retained by the current native verbosity.</summary>
	public static bool IsInfoEnabled => NativeApi.IsLogEnabled(2);
	/// <summary>Whether a warning would be retained by the current native verbosity.</summary>
	public static bool IsWarningEnabled => NativeApi.IsLogEnabled(5);
	/// <summary>Whether an error would be retained by the current native verbosity.</summary>
	public static bool IsErrorEnabled => NativeApi.IsLogEnabled(0);

	/// <summary>Reports gameplay information when native verbosity permits it.</summary>
	/// <param name="message">The diagnostic message.</param>
	public static void Info(string message) => NativeApi.WriteLog(2, message);
	/// <summary>Reports a recoverable gameplay problem when native verbosity permits it.</summary>
	/// <param name="message">The diagnostic message.</param>
	public static void Warning(string message) => NativeApi.WriteLog(5, message);
	/// <summary>Reports a gameplay failure when native verbosity permits it.</summary>
	/// <param name="message">The diagnostic message.</param>
	public static void Error(string message) => NativeApi.WriteLog(0, message);
}
