using Lion.Engine.Internal;
using System.Numerics;

namespace Lion.Engine;

/// <summary>A non-owning reference to one native scene lifetime.</summary>
/// <remarks>Engine-thread-only. Clearing or reloading a scene invalidates this reference, even if
/// native tools retain the old Scene object. Scenes own their entities; this value owns neither.</remarks>
public readonly struct Scene
{
	private readonly ulong _handle;
	internal Scene(ulong handle) => _handle = handle;
	internal ulong Handle => _handle;

	/// <summary>The native world's gravity in meters per second squared.</summary>
	public Vector2 Gravity
	{
		get { Span<float> v = stackalloc float[10]; NativeApi.SceneCommand(_handle, 0, v); return new(v[0], v[1]); }
		set { Span<float> v = stackalloc float[10]; v[0] = value.X; v[1] = value.Y; NativeApi.SceneCommand(_handle, 1, v); }
	}
	/// <summary>The current number of scene-owned entities.</summary>
	public int EntityCount { get { Span<float> v = stackalloc float[10]; return checked((int)NativeApi.SceneCommand(_handle, 2, v)); } }
	/// <summary>Finds the first native trait in scene order, including inactive owners.</summary>
	public T? FindComponent<T>() where T : Component
	{
		Span<float> v = stackalloc float[10]; v[0] = ComponentType<T>.Kind;
		ulong token = NativeApi.SceneCommand(_handle, 20, v);
		return token == 0 ? null : ComponentType<T>.Create(new Entity(token));
	}
	/// <summary>Counts active owners carrying a native trait, matching the C++ scene query.</summary>
	public int CountActiveComponents<T>() where T : Component
	{
		Span<float> v = stackalloc float[10]; v[0] = ComponentType<T>.Kind; NativeApi.SceneCommand(_handle, 21, v); return checked((int)v[0]);
	}
	/// <summary>Returns an entity in scene order; enumerating by index is for setup, not hot loops.</summary>
	public Entity GetEntity(int index) { ArgumentOutOfRangeException.ThrowIfNegative(index); Span<float> v = stackalloc float[10]; return new Entity(NativeApi.SceneCommand(_handle, 3, v, (ulong)index)); }
	/// <summary>Instantiates a rooted Assembly with its complete authored hierarchy.</summary>
	public Entity Instantiate(string path)
	{
		if (ScriptRuntime.CurrentCallback == 6) throw new InvalidOperationException("Cannot instantiate during destruction.");
		Span<float> v = stackalloc float[10]; return new Entity(NativeApi.SceneCommand(_handle, 4, v, path: path));
	}
	/// <summary>Pauses active scene gameplay and physics; opted-in behaviours and native widgets still update.</summary>
	public bool IsPaused { get { Span<float> v = stackalloc float[10]; NativeApi.SceneCommand(_handle, 5, v); return v[0] != 0; } set { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; NativeApi.SceneCommand(_handle, 6, v); } }
	/// <summary>Sets pause even when this scene handle is returned by Entity.Scene.</summary>
	public void SetPaused(bool value) { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; NativeApi.SceneCommand(_handle, 6, v); }
	/// <summary>Casts a pixel-space segment through the native physics world without allocating on a miss.</summary>
	public bool Raycast(Vector2 origin, Vector2 translation, out RaycastHit hit, Entity ignore = default)
	{
		Span<float> v = stackalloc float[10]; v[0] = origin.X; v[1] = origin.Y; v[2] = translation.X; v[3] = translation.Y;
		ulong token = NativeApi.SceneCommand(_handle, 7, v, ignore.Handle);
		hit = token == 0 ? default : new RaycastHit(new Entity(token), new(v[0], v[1]), new(v[2], v[3]), v[4]);
		return token != 0;
	}
	/// <summary>Queries a hit normal without allocating an Entity/Transform view, suitable for grounded checks.</summary>
	public bool RaycastNormal(Vector2 origin, Vector2 translation, out Vector2 normal, Entity ignore = default)
	{
		Span<float> v = stackalloc float[10]; v[0] = origin.X; v[1] = origin.Y; v[2] = translation.X; v[3] = translation.Y;
		ulong token = NativeApi.SceneCommand(_handle, 7, v, ignore.Handle);
		normal = new(v[2], v[3]); return token != 0;
	}
	/// <summary>Obtains a native mixer view for this scene lifetime; cache it during setup.</summary>
	public AudioMixer Audio => new(this);

	/// <summary>Whether this scene lifetime still exists. The default reference is invalid.</summary>
	public bool IsValid => NativeApi.IsSceneValid(_handle);

	/// <summary>Finds the first entity with an exact authored name, including disabled entities.</summary>
	/// <param name="name">Case-sensitive name, not an asset path.</param>
	/// <returns>A non-owning entity reference, or null when no entity matches.</returns>
	/// <remarks>Cache lookups outside update loops: a successful lookup creates a Transform wrapper.</remarks>
	public Entity? FindEntity(string name)
	{
		var result = NativeApi.SceneEntity(_handle, name, false);
		return result.Handle == 0 ? null : result;
	}

	/// <summary>Creates a bare native entity with identity placement in this scene.</summary>
	/// <param name="name">Initial authored name.</param>
	/// <returns>The new scene-owned entity.</returns>
	/// <remarks>This allocates. Author reusable render/physics/script composition in the editor;
/// use AddComponent initializers for dynamic composition.</remarks>
	public Entity CreateEntity(string name = "Entity") => NativeApi.SceneEntity(_handle, name, true);

	/// <summary>Requests a scene transition at the native end-of-update boundary.</summary>
	/// <param name="path">Assets-relative .lnscene path using forward slashes.</param>
	/// <remarks>Call from OnUpdateBegin, OnUpdate or OnUpdateEnd on the active scene. Construction,
/// Awake and destruction cannot request transitions. References into the old scene become invalid
/// after the transition; reacquire them in the next scene's scripts.</remarks>
	public void Load(string path) => NativeApi.RequestScene(_handle, path);
}

/// <summary>A native raycast result, with world-pixel position and unit normal.</summary>
public readonly record struct RaycastHit(Entity Entity, Vector2 Position, Vector2 Normal, float Fraction);
