using System.Numerics;
using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>The native host window. Mutating it in Mane Play is rejected so scripts cannot modify the editor shell.</summary>
public static class Window
{
	/// <summary>The fixed logical game frame, independent of native window resolution.</summary>
	public static Vector2 LogicalSize => new(1280, 720);
	/// <summary>The current native host dimensions.</summary>
	public static Vector2 Size { get { Span<float> v = stackalloc float[10]; NativeApi.HostCommand(1, v); return new(v[0], v[1]); } }
	/// <summary>The standalone player's base window title.</summary>
	public static string Title { get => NativeApi.WindowTitle(); set { Span<float> v = stackalloc float[10]; NativeApi.HostCommand(11, v, value); } }
	/// <summary>The native clear color.</summary>
	public static Vector3 BackgroundColor { get { Span<float> v = stackalloc float[10]; NativeApi.HostCommand(3, v); return new(v[0], v[1], v[2]); } set { Span<float> v = stackalloc float[10]; v[0] = value.X; v[1] = value.Y; v[2] = value.Z; NativeApi.HostCommand(4, v); } }
	/// <summary>Whether the native host is maximized.</summary>
	public static bool IsMaximized { get { Span<float> v = stackalloc float[10]; NativeApi.HostCommand(14, v); return v[0] != 0; } }
	/// <summary>Whether the native frame-stat title overlay is enabled.</summary>
	public static bool ShowFrameStats { get { Span<float> v = stackalloc float[10]; NativeApi.HostCommand(8, v); return v[0] != 0; } set { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; NativeApi.HostCommand(9, v); } }
	/// <summary>Resizes the standalone player. Dimensions must be from 1 through 16384 pixels.</summary>
	public static void SetSize(int width, int height) { Span<float> v = stackalloc float[10]; v[0] = width; v[1] = height; NativeApi.HostCommand(2, v); }
	/// <summary>Changes standalone resize capability.</summary>
	public static void SetResizable(bool value) { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; NativeApi.HostCommand(5, v); }
	/// <summary>Maximizes or restores the standalone player.</summary>
	public static void SetMaximized(bool value) { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; NativeApi.HostCommand(6, v); }
	/// <summary>Minimizes the standalone player.</summary>
	public static void Minimize() { Span<float> v = stackalloc float[10]; NativeApi.HostCommand(7, v); }
	/// <summary>Sets a resource-relative player window icon.</summary>
	public static void SetIcon(string path) { ResourcePath.Validate(path); Span<float> v = stackalloc float[10]; NativeApi.HostCommand(12, v, path); }
}
