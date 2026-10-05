using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A non-owning reference to one native scene lifetime.</summary>
/// <remarks>Engine-thread-only. Clearing or reloading a scene invalidates this reference, even if
/// native tools retain the old Scene object. Scenes own their entities; this value owns neither.</remarks>
public readonly struct Scene
{
	private readonly ulong _handle;
	internal Scene(ulong handle) => _handle = handle;

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
/// runtime component mutation and Assembly instantiation are not exposed by this increment.</remarks>
	public Entity CreateEntity(string name = "Entity") => NativeApi.SceneEntity(_handle, name, true);

	/// <summary>Requests a scene transition at the native end-of-update boundary.</summary>
	/// <param name="path">Assets-relative .lnscene path using forward slashes.</param>
	/// <remarks>Call from OnUpdateBegin, OnUpdate or OnUpdateEnd on the active scene. Construction,
/// Awake and destruction cannot request transitions. References into the old scene become invalid
/// after the transition; reacquire them in the next scene's scripts.</remarks>
	public void Load(string path) => NativeApi.RequestScene(_handle, path);
}
