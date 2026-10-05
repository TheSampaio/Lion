using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Controls scene-authored bitmap text using Lion's native sprite-batch renderer.</summary>
public sealed class TextRenderer : Component
{
	internal TextRenderer(Entity entity) : base(entity, 0) { }

	/// <summary>UTF-8-capable display text. Changing it rebuilds native glyphs on demand.</summary>
	/// <remarks>Reading allocates a string; update scores and labels only when their values change.
/// Font, alignment, size and colour remain authored in the editor.</remarks>
	public string Text
	{
		get => NativeApi.ReadText(Entity.Handle, 1);
		set => NativeApi.WriteText(Entity.Handle, 1, value);
	}
}
