using System.Numerics;
using System.Runtime.InteropServices;
using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A local 2D Transform snapshot: pixel position, degree rotation, and dimensionless scale.</summary>
/// <remarks>Reading a snapshot does not keep it synchronized. Write it back through Transform.State.
/// Values must be finite; negative and zero scales have the same meaning as in the native engine.</remarks>
[StructLayout(LayoutKind.Sequential)]
public struct TransformState
{
	/// <summary>Local position in pixels, with positive Y pointing upward.</summary>
	public Vector2 Position;

	/// <summary>Local rotation in degrees about the 2D plane's normal.</summary>
	public float Rotation;

	/// <summary>Local dimensionless scale factors for the X and Y axes.</summary>
	public Vector2 Scale;
}

/// <summary>Non-owning access to a native entity's local 2D placement.</summary>
/// <remarks>Use State to read or write several fields in one native call. A property setter reads
/// a snapshot and writes it back. The wrapper is cached at attachment; invalid lifetimes throw.</remarks>
public sealed class Transform
{
	private readonly ulong _entityHandle;
	internal Transform(ulong entityHandle) => _entityHandle = entityHandle;

	/// <summary>Reads or writes all local Transform fields in a single call to the native core.</summary>
	public TransformState State
	{
		get => NativeApi.GetTransform(_entityHandle);
		set => NativeApi.SetTransform(_entityHandle, in value);
	}

	/// <summary>Local pixel position relative to the parent; positive Y points upward.</summary>
	public Vector2 Position
	{
		get => State.Position;
		set { var state = State; state.Position = value; State = state; }
	}

	/// <summary>Local rotation in degrees, without altering the parent Transform.</summary>
	public float Rotation
	{
		get => State.Rotation;
		set { var state = State; state.Rotation = value; State = state; }
	}

	/// <summary>Local dimensionless X/Y scale relative to the parent.</summary>
	public Vector2 Scale
	{
		get => State.Scale;
		set { var state = State; state.Scale = value; State = state; }
	}
}
