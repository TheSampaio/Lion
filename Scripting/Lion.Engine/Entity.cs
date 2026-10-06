using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A non-owning value referring to one lifetime of a native entity.</summary>
/// <remarks>The default value is invalid. These values do not keep scenes or entities alive.
/// All queries and operations are engine-thread-only.</remarks>
public readonly struct Entity : IEquatable<Entity>
{
	private readonly Transform? _transform;
	private readonly Transform? _worldTransform;
	internal Entity(ulong handle)
	{
		Handle = handle;
		_transform = new Transform(handle);
		_worldTransform = new Transform(handle, true);
	}
	internal ulong Handle { get; }

	/// <summary>Whether this lifetime still exists. False after destruction or runtime shutdown.</summary>
	public bool IsValid => NativeApi.IsEntityValid(Handle);

	/// <summary>Accesses the cached local 2D Transform wrapper without allocating on each read.</summary>
	public Transform Transform => _transform ?? throw new InvalidOperationException("The Entity has not been attached.");
	/// <summary>The composed world transform, cached like the local transform.</summary>
	public Transform WorldTransform => _worldTransform ?? throw new InvalidOperationException("The Entity has not been attached.");
	/// <summary>The parent, or an invalid value for a root.</summary>
	public Entity Parent { get { TransformState state = default; return new Entity(NativeApi.Hierarchy(Handle, 0, ref state)); } }
	/// <summary>The current number of direct children.</summary>
	public int ChildCount { get { TransformState state = default; return checked((int)NativeApi.Hierarchy(Handle, 3, ref state)); } }
	/// <summary>Returns one direct child in native hierarchy order.</summary>
	public Entity GetChild(int index) { ArgumentOutOfRangeException.ThrowIfNegative(index); TransformState state = default; return new Entity(NativeApi.Hierarchy(Handle, 4, ref state, (ulong)index)); }
	/// <summary>Reparents within the same scene. Cycles are rejected; default removes the parent.</summary>
	public void SetParent(Entity parent = default, bool keepWorldTransform = true) { TransformState state = default; NativeApi.Hierarchy(Handle, keepWorldTransform ? 1 : 2, ref state, parent.Handle); }
	/// <summary>Moves this owner's scene order before another owner, or to the end for a default handle.</summary>
	public void MoveBefore(Entity before = default) { TransformState state = default; NativeApi.Hierarchy(Handle, 7, ref state, before.Handle); }

	/// <summary>The scene that owns this lifetime. Clearing it invalidates the returned reference.</summary>
	public Scene Scene => NativeApi.SceneOf(Handle);

	/// <summary>The exact authored name. Reading allocates its managed string.</summary>
	public string Name { get => NativeApi.ReadText(Handle, 0); set => NativeApi.WriteText(Handle, 0, value); }

	/// <summary>Whether this entity is enabled directly; native enable/disable callbacks follow changes.</summary>
	public bool IsEnabled { get => NativeApi.GetEntityState(Handle, 0); set => NativeApi.SetEntityState(Handle, 0, value); }

	/// <summary>Whether this entity is drawn directly. Hidden entities still simulate.</summary>
	public bool IsVisible { get => NativeApi.GetEntityState(Handle, 1); set => NativeApi.SetEntityState(Handle, 1, value); }
	/// <summary>Sets native enabled state, including when this handle is returned by a property.</summary>
	public void SetEnabled(bool value) => NativeApi.SetEntityState(Handle, 0, value);
	/// <summary>Sets native visibility, including when this handle is returned by a property.</summary>
	public void SetVisible(bool value) => NativeApi.SetEntityState(Handle, 1, value);

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
	/// <summary>Finds an already-awake managed behaviour by its registered full type name.</summary>
	public T? GetBehaviour<T>() where T : Behaviour => NativeApi.Behaviour<T>(Handle, 0);
	/// <summary>Attaches a registered managed behaviour and immediately runs its native-driven Awake.</summary>
	public T AddBehaviour<T>() where T : Behaviour
	{
		if (ScriptRuntime.CurrentCallback == 6) throw new InvalidOperationException("Cannot attach behaviours during destruction.");
		return NativeApi.Behaviour<T>(Handle, 1) ?? throw new InvalidOperationException("The behaviour could not be created.");
	}
	/// <summary>Defers removal, including self-removal, until the scene's safe frame boundary.</summary>
	public void RemoveBehaviour<T>() where T : Behaviour => NativeApi.Behaviour<T>(Handle, 2);

	/// <summary>Attaches a new native trait, initializes it, then runs its native Awake.</summary>
	/// <remarks>Configure body type and collider size in initialize, before their simulation objects exist.
	/// Dependencies follow the native registry. Adding an existing trait is rejected.
	/// If initialization fails, Awake is skipped and the new trait is removed at the safe frame boundary.</remarks>
	public T AddComponent<T>(Action<T>? initialize = null) where T : Component
	{
		if (ScriptRuntime.CurrentCallback == 6) throw new InvalidOperationException("Cannot attach traits during destruction.");
		if (HasComponent<T>()) throw new InvalidOperationException($"{typeof(T).Name} is already attached.");
		Span<float> values = stackalloc float[10];
		NativeApi.Command(Handle, ComponentType<T>.Kind, 0, values);
		var component = ComponentType<T>.Create(this);
		try
		{
			initialize?.Invoke(component);
			values.Clear(); NativeApi.Command(Handle, ComponentType<T>.Kind, 1, values);
		}
		catch
		{
			values.Clear(); NativeApi.Command(Handle, ComponentType<T>.Kind, 2, values);
			throw;
		}
		return component;
	}
	/// <summary>Defers native trait removal until the scene finishes its current update and physics.</summary>
	public void RemoveComponent<T>() where T : Component
	{
		if (!HasComponent<T>()) return;
		Span<float> values = stackalloc float[10]; NativeApi.Command(Handle, ComponentType<T>.Kind, 2, values);
	}
	/// <summary>Compares lifetime identity, not names or positions.</summary>
	public bool Equals(Entity other) => Handle == other.Handle;
	/// <inheritdoc />
	public override bool Equals(object? obj) => obj is Entity other && Equals(other);
	/// <inheritdoc />
	public override int GetHashCode() => Handle.GetHashCode();
	/// <summary>Compares native lifetime tokens.</summary>
	public static bool operator ==(Entity left, Entity right) => left.Equals(right);
	/// <summary>Compares native lifetime tokens.</summary>
	public static bool operator !=(Entity left, Entity right) => !left.Equals(right);

	/// <summary>Requests native removal of this entity and its subtree at the end of the frame.</summary>
	/// <remarks>The reference remains valid during the current callback, then becomes invalid after
	/// removals are flushed. Repeated requests are safe; this does not immediately destroy an executing script.</remarks>
	public void Destroy() => NativeApi.DestroyEntity(Handle);
}
