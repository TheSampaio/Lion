using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;

namespace Lion.Engine.Internal;

[StructLayout(LayoutKind.Sequential)]
internal struct PropertyValue
{
	internal float X, Y, Z, W;
	internal int Integer, Kind;
}

internal static unsafe partial class NativeApi
{
	internal static void HostCommand(int operation, Span<float> values, string? text = null)
	{
		CheckThread();
		if (values.Length != 10) throw new ArgumentException("A host command requires ten values.");
		if (text != null) CheckString(text);
		int required;
		fixed (float* pointer = values)
		fixed (byte* utf8 = text == null ? null : Encoding.UTF8.GetBytes(text + '\0'))
			CheckStatus(_functions.HostCommand(operation, pointer, utf8, 0, &required));
	}
	internal static string WindowTitle()
	{
		CheckThread();
		float* values = stackalloc float[10]; int required;
		CheckStatus(_functions.HostCommand(10, values, null, 0, &required));
		if (required < 1 || required > 1_048_576) throw new InvalidOperationException("Window title exceeds the supported size.");
		byte[] buffer = new byte[required];
		fixed (byte* output = buffer) CheckStatus(_functions.HostCommand(10, values, output, buffer.Length, &required));
		return Encoding.UTF8.GetString(buffer, 0, required - 1);
	}
	internal static byte[] ReadResource(string path)
	{
		CheckThread(); ResourcePath.Validate(path);
		byte[] name = Encoding.UTF8.GetBytes(path + '\0');
		float* values = stackalloc float[10]; int required;
		fixed (byte* text = name) CheckStatus(_functions.HostCommand(13, values, text, 0, &required));
		if (required < 0 || required > 64 * 1024 * 1024) throw new InvalidOperationException("Resource exceeds the supported size.");
		byte[] buffer = new byte[Math.Max(required, name.Length)]; name.CopyTo(buffer, 0); values[0] = 1;
		fixed (byte* output = buffer) CheckStatus(_functions.HostCommand(13, values, output, buffer.Length, &required));
		if (buffer.Length != required) Array.Resize(ref buffer, required);
		return buffer;
	}
	internal static T? Behaviour<T>(ulong entity, int operation) where T : Engine.Behaviour
	{
		CheckThread();
		fixed (byte* name = Encoding.UTF8.GetBytes(typeof(T).FullName + '\0'))
		{
			ulong instance;
			CheckStatus(_functions.Behaviour(entity, name, operation, &instance));
			return ScriptRuntime.GetBehaviour<T>(instance);
		}
	}
	internal static void Command(ulong entity, int kind, int operation, Span<float> values)
	{
		CheckThread();
		if (values.Length != 10) throw new ArgumentException("A native command requires ten values.");
		fixed (float* pointer = values) CheckStatus(_functions.ComponentCommand(entity, kind, operation, pointer));
	}

	internal static ulong SceneCommand(ulong scene, int operation, Span<float> values, ulong other = 0, string? path = null)
	{
		CheckThread();
		if (values.Length != 10) throw new ArgumentException("A scene command requires ten values.");
		if (path != null) ResourcePath.Validate(path);
		fixed (byte* text = path == null ? null : Encoding.UTF8.GetBytes(path + '\0'))
		fixed (float* pointer = values) CheckStatus(_functions.SceneCommand(scene, operation, &other, pointer, text));
		return other;
	}

	internal static ulong Hierarchy(ulong entity, int operation, ref TransformState state, ulong other = 0)
	{
		CheckThread();
		fixed (TransformState* pointer = &state) CheckStatus(_functions.Hierarchy(entity, operation, &other, pointer));
		return other;
	}

	internal static TransformState GetWorldTransform(ulong entity)
	{
		TransformState state = default;
		Hierarchy(entity, 5, ref state);
		return state;
	}
	internal static void SetWorldTransform(ulong entity, TransformState state) => Hierarchy(entity, 6, ref state);

	internal static Vector2 InputQuery(ulong action, int operation, int code = 0, int gamepad = 0)
	{
		CheckThread();
		float* values = stackalloc float[2];
		CheckStatus(_functions.InputQuery(action, operation, code, gamepad, values));
		return new Vector2(values[0], values[1]);
	}

	internal static T Field<T>(ulong entity, int kind, string name, T value, bool write)
	{
		CheckThread();
		CheckString(name);
		PropertyValue property = default;
		object? boxed = value;
		if (typeof(T) == typeof(float)) { property.Kind = 0; if (write) property.X = (float)boxed!; }
		else if (typeof(T) == typeof(int)) { property.Kind = 1; if (write) property.Integer = (int)boxed!; }
		else if (typeof(T) == typeof(bool)) { property.Kind = 2; if (write) property.Integer = (bool)boxed! ? 1 : 0; }
		else if (typeof(T) == typeof(string)) { property.Kind = 3; if (write) CheckString((string)boxed!); }
		else if (typeof(T) == typeof(Vector3)) { property.Kind = 4; if (write) { var v = (Vector3)boxed!; property.X = v.X; property.Y = v.Y; property.Z = v.Z; } }
		else if (typeof(T) == typeof(Vector2)) { property.Kind = 5; if (write) { var v = (Vector2)boxed!; property.X = v.X; property.Y = v.Y; } }
		else throw new NotSupportedException($"{typeof(T).Name} is not a native reflected field type.");
		int required = 0;
		fixed (byte* fieldName = Encoding.UTF8.GetBytes(name + '\0'))
		fixed (byte* text = write && property.Kind == 3 ? Encoding.UTF8.GetBytes((string)boxed! + '\0') : null)
		{
			CheckStatus(_functions.ComponentField(entity, kind, fieldName, &property, text, 0, &required, write ? 1 : 0));
			if (write) return value;
			if (property.Kind == 3)
			{
				if (required < 1 || required > 1_048_576) throw new InvalidOperationException("Native field text exceeds the supported size.");
				byte[] buffer = new byte[required];
				fixed (byte* output = buffer) CheckStatus(_functions.ComponentField(entity, kind, fieldName, &property, output, buffer.Length, &required, 0));
				return (T)(object)Encoding.UTF8.GetString(buffer, 0, required - 1);
			}
		}
		object result = property.Kind switch
		{
			0 => property.X, 1 => property.Integer, 2 => property.Integer != 0,
			4 => new Vector3(property.X, property.Y, property.Z), 5 => new Vector2(property.X, property.Y),
			_ => throw new InvalidOperationException("Unsupported reflected type.")
		};
		return (T)result;
	}
}
