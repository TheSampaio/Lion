using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Controls the native sprite trait without owning its texture or renderer.</summary>
public sealed class SpriteRenderer : Component
{
	internal SpriteRenderer(Entity entity) : base(entity, 1) { }

	/// <summary>The texture resource identity. Assignment reuses Lion's shared native asset cache.</summary>
	/// <remarks>The default empty asset clears the texture. Reading allocates its identity string.</remarks>
	public TextureAsset Texture
	{
		get { string path = NativeApi.ReadText(Entity.Handle, 2); return path.Length == 0 ? default : TextureAsset.FromPath(path); }
		set => NativeApi.WriteText(Entity.Handle, 2, value.Path);
	}

	/// <summary>Draw order: lower values render behind higher values.</summary>
	public int Order { get => NativeApi.GetComponentState(Entity.Handle, 1, 1); set => NativeApi.SetComponentState(Entity.Handle, 1, 1, value); }

	/// <summary>Mirrors the texture horizontally without changing Transform or physics.</summary>
	public bool FlipX { get => NativeApi.GetComponentState(Entity.Handle, 1, 2) != 0; set => NativeApi.SetComponentState(Entity.Handle, 1, 2, value ? 1 : 0); }

	/// <summary>Mirrors the texture vertically without changing Transform or physics.</summary>
	public bool FlipY { get => NativeApi.GetComponentState(Entity.Handle, 1, 3) != 0; set => NativeApi.SetComponentState(Entity.Handle, 1, 3, value ? 1 : 0); }
}
