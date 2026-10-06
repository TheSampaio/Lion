using System.Runtime.InteropServices;
using System.Text;

namespace Lion.Engine.Internal;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeFunctions
{
	internal uint Version;
	internal uint Size;
	internal delegate* unmanaged[Cdecl]<ulong, int> ValidateEntity;
	internal delegate* unmanaged[Cdecl]<ulong, TransformState*, int> GetTransform;
	internal delegate* unmanaged[Cdecl]<ulong, TransformState*, int> SetTransform;
	internal delegate* unmanaged[Cdecl]<byte*, ulong*, int> ResolveAction;
	internal delegate* unmanaged[Cdecl]<ulong, float*, int> ReadAction;
	internal delegate* unmanaged[Cdecl]<int, int> IsLogEnabled;
	internal delegate* unmanaged[Cdecl]<int, byte*, void> WriteLog;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, void> ReportError;
	internal delegate* unmanaged[Cdecl]<ulong, ulong*, int> SceneOf;
	internal delegate* unmanaged[Cdecl]<ulong, int> ValidateScene;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, ulong*, int> FindSceneEntity;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, ulong*, int> CreateSceneEntity;
	internal delegate* unmanaged[Cdecl]<ulong, int> DestroyEntity;
	internal delegate* unmanaged[Cdecl]<ulong, int, int*, int> GetEntityState;
	internal delegate* unmanaged[Cdecl]<ulong, int, int, int> SetEntityState;
	internal delegate* unmanaged[Cdecl]<ulong, int, byte*, int, int*, int> ReadText;
	internal delegate* unmanaged[Cdecl]<ulong, int, byte*, int> WriteText;
	internal delegate* unmanaged[Cdecl]<ulong, int, int*, int> HasComponent;
	internal delegate* unmanaged[Cdecl]<ulong, int, int, int*, int> GetComponentState;
	internal delegate* unmanaged[Cdecl]<ulong, int, int, int, int> SetComponentState;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, int> RequestScene;
	internal delegate* unmanaged[Cdecl]<int> Quit;
	internal delegate* unmanaged[Cdecl]<ulong, int, int, float*, int> ComponentCommand;
	internal delegate* unmanaged[Cdecl]<ulong, int, byte*, PropertyValue*, byte*, int, int*, int, int> ComponentField;
	internal delegate* unmanaged[Cdecl]<ulong, int, ulong*, TransformState*, int> Hierarchy;
	internal delegate* unmanaged[Cdecl]<ulong, int, ulong*, float*, byte*, int> SceneCommand;
	internal delegate* unmanaged[Cdecl]<ulong, int, int, int, float*, int> InputQuery;
	internal delegate* unmanaged[Cdecl]<ulong, byte*, int, ulong*, int> Behaviour;
	internal delegate* unmanaged[Cdecl]<int, float*, byte*, int, int*, int> HostCommand;
}

internal static unsafe partial class NativeApi
{
	private static NativeFunctions _functions;
	private static int _threadId;

	internal static bool Bind(NativeFunctions* functions)
	{
		if (functions == null || functions->Version != 4 || functions->Size != sizeof(NativeFunctions)
			|| functions->ValidateEntity == null || functions->GetTransform == null
			|| functions->SetTransform == null || functions->ResolveAction == null
			|| functions->ReadAction == null || functions->IsLogEnabled == null
			|| functions->WriteLog == null || functions->ReportError == null
			|| functions->SceneOf == null || functions->ValidateScene == null || functions->FindSceneEntity == null
			|| functions->CreateSceneEntity == null || functions->DestroyEntity == null
			|| functions->GetEntityState == null || functions->SetEntityState == null
			|| functions->ReadText == null || functions->WriteText == null || functions->HasComponent == null
			|| functions->GetComponentState == null || functions->SetComponentState == null
			|| functions->RequestScene == null || functions->Quit == null || functions->ComponentCommand == null
			|| functions->ComponentField == null || functions->Hierarchy == null || functions->SceneCommand == null || functions->InputQuery == null || functions->Behaviour == null || functions->HostCommand == null)
		{
			return false;
		}
		_functions = *functions;
		_threadId = Environment.CurrentManagedThreadId;
		return true;
	}

	internal static void CheckThread()
	{
		if (Environment.CurrentManagedThreadId != _threadId)
		{
			throw new InvalidOperationException("Lion gameplay APIs require the engine thread.");
		}
	}

	internal static void CheckStatus(int status)
	{
		if (status != 0)
		{
			throw new InvalidOperationException($"Lion native operation failed (status {status}): the lifetime or runtime is unavailable.");
		}
	}

	internal static bool IsEntityValid(ulong handle)
	{
		CheckThread();
		return _functions.ValidateEntity(handle) == 0;
	}

	internal static TransformState GetTransform(ulong handle)
	{
		CheckThread();
		TransformState result;
		CheckStatus(_functions.GetTransform(handle, &result));
		return result;
	}

	internal static void SetTransform(ulong handle, in TransformState value)
	{
		CheckThread();
		var state = value;
		if (!float.IsFinite(state.Position.X) || !float.IsFinite(state.Position.Y)
			|| !float.IsFinite(state.Rotation) || !float.IsFinite(state.Scale.X) || !float.IsFinite(state.Scale.Y))
		{
			throw new ArgumentOutOfRangeException(nameof(value), "Transform fields must be finite.");
		}
		CheckStatus(_functions.SetTransform(handle, &state));
	}

	internal static InputAction ResolveAction(string name)
	{
		CheckThread();
		if (name.Contains('\0'))
		{
			throw new ArgumentException("Action names cannot contain null characters.", nameof(name));
		}
		fixed (byte* text = Encoding.UTF8.GetBytes(name + '\0'))
		{
			ulong handle;
			CheckStatus(_functions.ResolveAction(text, &handle));
			return new InputAction(handle);
		}
	}

	internal static float ReadAction(ulong handle)
	{
		CheckThread();
		float result;
		CheckStatus(_functions.ReadAction(handle, &result));
		return result;
	}

	internal static bool IsLogEnabled(int level)
	{
		CheckThread();
		return _functions.IsLogEnabled(level) != 0;
	}

	internal static void WriteLog(int level, string message)
	{
		ArgumentNullException.ThrowIfNull(message);
		if (!IsLogEnabled(level))
		{
			return;
		}
		fixed (byte* text = Encoding.UTF8.GetBytes(message + '\0'))
		{
			_functions.WriteLog(level, text);
		}
	}

	internal static void ReportError(ulong entity, string message)
	{
		fixed (byte* text = Encoding.UTF8.GetBytes(message + '\0'))
		{
			_functions.ReportError(entity, text);
		}
	}

	internal static void CheckString(string value)
	{
		ArgumentNullException.ThrowIfNull(value);
		if (value.Contains('\0')) { throw new ArgumentException("Lion strings cannot contain null characters.", nameof(value)); }
	}

	internal static Scene SceneOf(ulong handle)
	{
		CheckThread();
		ulong result;
		CheckStatus(_functions.SceneOf(handle, &result));
		return new Scene(result);
	}

	internal static bool IsSceneValid(ulong handle) { CheckThread(); return _functions.ValidateScene(handle) == 0; }
	internal static Entity SceneEntity(ulong handle, string name, bool create)
	{
		if (create && ScriptRuntime.CurrentCallback == 6) throw new InvalidOperationException("Cannot create entities during destruction.");
		CheckThread();
		CheckString(name);
		fixed (byte* text = Encoding.UTF8.GetBytes(name + '\0'))
		{
			ulong result;
			CheckStatus(create ? _functions.CreateSceneEntity(handle, text, &result) : _functions.FindSceneEntity(handle, text, &result));
			return result == 0 ? default : new Entity(result);
		}
	}
	internal static void DestroyEntity(ulong handle) { CheckThread(); CheckStatus(_functions.DestroyEntity(handle)); }
	internal static bool GetEntityState(ulong handle, int property)
	{
		CheckThread();
		int value;
		CheckStatus(_functions.GetEntityState(handle, property, &value));
		return value != 0;
	}
	internal static void SetEntityState(ulong handle, int property, bool value)
	{
		CheckThread();
		CheckStatus(_functions.SetEntityState(handle, property, value ? 1 : 0));
	}
	internal static string ReadText(ulong handle, int property)
	{
		CheckThread();
		int length;
		CheckStatus(_functions.ReadText(handle, property, null, 0, &length));
		if (length < 1 || length > 1_048_576) { throw new InvalidOperationException("Native text exceeds the supported size."); }
		Span<byte> buffer = length <= 512 ? stackalloc byte[length] : new byte[length];
		fixed (byte* text = buffer)
		{
			CheckStatus(_functions.ReadText(handle, property, text, length, &length));
			return Encoding.UTF8.GetString(buffer[..(length - 1)]);
		}
	}
	internal static void WriteText(ulong handle, int property, string value)
	{
		CheckThread();
		CheckString(value);
		fixed (byte* text = Encoding.UTF8.GetBytes(value + '\0')) { CheckStatus(_functions.WriteText(handle, property, text)); }
	}
	internal static bool HasComponent(ulong handle, int kind)
	{
		CheckThread();
		int value;
		CheckStatus(_functions.HasComponent(handle, kind, &value));
		return value != 0;
	}
	internal static int GetComponentState(ulong handle, int kind, int property)
	{
		CheckThread();
		int value;
		CheckStatus(_functions.GetComponentState(handle, kind, property, &value));
		return value;
	}
	internal static void SetComponentState(ulong handle, int kind, int property, int value)
	{
		CheckThread();
		CheckStatus(_functions.SetComponentState(handle, kind, property, value));
	}
	internal static void RequestScene(ulong handle, string path)
	{
		CheckThread();
		if (ScriptRuntime.CurrentCallback is not (3 or 4 or 5 or 7))
		{
			throw new InvalidOperationException("Request scene changes from an update callback, never during construction or destruction.");
		}
		ResourcePath.Validate(path);
		fixed (byte* text = Encoding.UTF8.GetBytes(path + '\0')) { CheckStatus(_functions.RequestScene(handle, text)); }
	}
	internal static void Quit() { CheckThread(); CheckStatus(_functions.Quit()); }
}
