using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A cached reference to a named action evaluated by the native project input map.</summary>
/// <remarks>Resolve it once during OnAwake, not every frame. It expires on scripting runtime shutdown.</remarks>
public readonly struct InputAction
{
	private readonly ulong _handle;
	internal InputAction(ulong handle) => _handle = handle;

	/// <summary>The current signed strength from the native bindings, including its configured deadzone.</summary>
	/// <remarks>A name absent from the current action map evaluates to zero, matching native Input.</remarks>
	public float Strength => NativeApi.ReadAction(_handle);
}

/// <summary>Access to Lion's native named action map on the engine thread.</summary>
public static class Input
{
	/// <summary>Interns an action name once for subsequent reads without per-frame UTF-8 marshaling.</summary>
	/// <param name="name">The exact name in the project's action map.</param>
	/// <returns>A value to cache for the lifetime of the scripting session.</returns>
	public static InputAction Action(string name)
	{
		ArgumentException.ThrowIfNullOrWhiteSpace(name);
		return NativeApi.ResolveAction(name);
	}
}
