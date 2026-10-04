namespace Lion.Engine;

/// <summary>Timing for the current native-driven script callback.</summary>
public static class Time
{
	[ThreadStatic]
	internal static float CallbackDeltaTime;

	/// <summary>The scene timestep in seconds during an update callback; zero outside one.</summary>
	public static float DeltaTime => CallbackDeltaTime;
}
