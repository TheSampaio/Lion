using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A non-owning value referring to one lifetime of a native entity.</summary>
/// <remarks>The default value is invalid. These values do not keep scenes or entities alive.
/// All queries and operations are engine-thread-only.</remarks>
public readonly struct Entity
{
	private readonly Transform? _transform;
	internal Entity(ulong handle)
	{
		Handle = handle;
		_transform = new Transform(handle);
	}
	internal ulong Handle { get; }

	/// <summary>Whether this lifetime still exists. False after destruction or runtime shutdown.</summary>
	public bool IsValid => NativeApi.IsEntityValid(Handle);

	/// <summary>Accesses the cached local 2D Transform wrapper without allocating on each read.</summary>
	public Transform Transform => _transform ?? throw new InvalidOperationException("The Entity has not been attached.");
}
