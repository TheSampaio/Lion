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
}

internal static unsafe class NativeApi
{
	private static NativeFunctions _functions;
	private static int _threadId;

	internal static bool Bind(NativeFunctions* functions)
	{
		if (functions == null || functions->Version != 1 || functions->Size != sizeof(NativeFunctions)
			|| functions->ValidateEntity == null || functions->GetTransform == null
			|| functions->SetTransform == null || functions->ResolveAction == null
			|| functions->ReadAction == null || functions->IsLogEnabled == null
			|| functions->WriteLog == null || functions->ReportError == null)
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
}
