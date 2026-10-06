using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Controls the current native game session, never an independently managed game loop.</summary>
public static class Application
{
	/// <summary>Whether the current native host is Mane rather than a standalone player.</summary>
	public static bool IsEditor { get { Span<float> values = stackalloc float[10]; Internal.NativeApi.HostCommand(0, values); return values[0] != 0; } }
	/// <summary>Requests the standalone player to close, or stops Play without closing Mane.</summary>
	/// <remarks>Engine-thread-only. Cleanup still runs through the native scene/component lifetime.</remarks>
	public static void RequestQuit() => NativeApi.Quit();
}
