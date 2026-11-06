using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A cached reference to a named action evaluated by the native project input map.</summary>
/// <remarks>Resolve it once during OnAwake, not every frame. It expires on scripting runtime shutdown.</remarks>
public readonly struct InputAction
{
	private readonly ulong _handle;
	internal InputAction(ulong handle) => _handle = handle;

	/// <summary>The current zero-to-one strength from the native bindings, including its configured deadzone.</summary>
	/// <remarks>A name absent from the current action map evaluates to zero, matching native Input.</remarks>
	public float Strength => NativeApi.ReadAction(_handle);
	/// <summary>True for the native action's press edge.</summary>
	public bool WasPressed => NativeApi.InputQuery(_handle, 0).X != 0;
}

/// <summary>Access to Lion's native named action map on the engine thread.</summary>
public static class Input
{
	/// <summary>The pointer in Lion's centered logical screen frame.</summary>
	public static System.Numerics.Vector2 PointerPosition => NativeApi.InputQuery(0, 1);
	/// <summary>Whether the last input family was a gamepad.</summary>
	public static bool IsGamepadInput => NativeApi.InputQuery(0, 2).X != 0;
	/// <summary>Tests one of the native gamepad slots.</summary>
	public static bool IsGamepadConnected(int gamepad = 0) => NativeApi.InputQuery(0, 3, gamepad: gamepad).X != 0;
	/// <summary>Reads a normalized native gamepad button code, 0 through 14.</summary>
	public static bool GetGamepadButton(int button, int gamepad = 0) => NativeApi.InputQuery(0, 4, button, gamepad).X != 0;
	/// <summary>Reads a normalized native gamepad axis code, 0 through 5.</summary>
	public static float GetGamepadAxis(int axis, int gamepad = 0) => NativeApi.InputQuery(0, 5, axis, gamepad).X;
	/// <summary>Reads a native key code's held state. Named actions are preferred for remappable gameplay.</summary>
	public static bool GetKey(int key) => NativeApi.InputQuery(0, 6, key).X != 0;
	/// <summary>Reads the native key tap, which fires on release after a press. Use InputAction.WasPressed for a press edge.</summary>
	public static bool GetKeyTap(int key) => NativeApi.InputQuery(0, 7, key).X != 0;
	/// <summary>Reads a native mouse button's held state.</summary>
	public static bool GetMouseButton(int button) => NativeApi.InputQuery(0, 8, button).X != 0;
	/// <summary>Interns an action name once for subsequent reads without per-frame UTF-8 marshaling.</summary>
	/// <param name="name">The exact name in the project's action map.</param>
	/// <returns>A value to cache for the lifetime of the scripting session.</returns>
	public static InputAction Action(string name)
	{
		ArgumentException.ThrowIfNullOrWhiteSpace(name);
		return NativeApi.ResolveAction(name);
	}
}
