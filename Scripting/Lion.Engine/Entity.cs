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

	/// <summary>The scene that owns this lifetime. Clearing it invalidates the returned reference.</summary>
	public Scene Scene => NativeApi.SceneOf(Handle);

	/// <summary>The exact authored name. Reading allocates its managed string.</summary>
	public string Name { get => NativeApi.ReadText(Handle, 0); set => NativeApi.WriteText(Handle, 0, value); }

	/// <summary>Whether this entity is enabled directly; native enable/disable callbacks follow changes.</summary>
	public bool IsEnabled { get => NativeApi.GetEntityState(Handle, 0); set => NativeApi.SetEntityState(Handle, 0, value); }

	/// <summary>Whether this entity is drawn directly. Hidden entities still simulate.</summary>
	public bool IsVisible { get => NativeApi.GetEntityState(Handle, 1); set => NativeApi.SetEntityState(Handle, 1, value); }

	/// <summary>Whether this entity and its ancestors are enabled.</summary>
	public bool IsActive => NativeApi.GetEntityState(Handle, 2);

	/// <summary>Tests for a supported native trait without allocating a wrapper.</summary>
	/// <typeparam name="T">A native Component binding such as TextRenderer or SpriteRenderer.</typeparam>
	/// <returns>True when the trait exists, including disabled traits.</returns>
	public bool HasComponent<T>() where T : Component => NativeApi.HasComponent(Handle, ComponentType<T>.Kind);

	/// <summary>Obtains a typed non-owning view, or null when this entity lacks the trait.</summary>
	/// <typeparam name="T">A native Component binding, not a managed Behaviour type.</typeparam>
	/// <returns>A newly allocated view. Cache it outside update loops.</returns>
	public T? GetComponent<T>() where T : Component => HasComponent<T>() ? ComponentType<T>.Create(this) : null;

	/// <summary>Requests native removal of this entity and its subtree at the end of the frame.</summary>
	/// <remarks>The reference remains valid during the current callback, then becomes invalid after
/// removals are flushed. Repeated requests are safe; this does not immediately destroy an executing script.</remarks>
	public void Destroy() => NativeApi.DestroyEntity(Handle);
}
