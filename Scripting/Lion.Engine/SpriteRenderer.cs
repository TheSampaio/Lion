using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Controls the native sprite trait without owning its texture or renderer.</summary>
public sealed class SpriteRenderer : Component
{
	/// <summary>Selects a normalized bottom-up atlas rectangle and its unscaled world-pixel size.</summary>
	public void SetRegion(System.Numerics.Vector2 minimum, System.Numerics.Vector2 maximum, System.Numerics.Vector2 size)
	{
		if (minimum.X < 0 || minimum.Y < 0 || maximum.X > 1 || maximum.Y > 1 || maximum.X <= minimum.X || maximum.Y <= minimum.Y || size.X <= 0 || size.Y <= 0) throw new ArgumentOutOfRangeException(nameof(size));
		Span<float> v = stackalloc float[10]; v[0] = minimum.X; v[1] = minimum.Y; v[2] = maximum.X; v[3] = maximum.Y; v[4] = size.X; v[5] = size.Y; Command(20, v);
	}
	/// <summary>Sets RGB modulation without managed allocation.</summary>
	public void SetColor(System.Numerics.Vector3 color) { Span<float> v = stackalloc float[10]; v[0] = color.X; v[1] = color.Y; v[2] = color.Z; Command(21, v); }
	/// <summary>The current region's pixel dimensions.</summary>
	public System.Numerics.Vector2 Size { get { Span<float> v = stackalloc float[10]; Command(22, v); return new(v[0], v[1]); } }
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
