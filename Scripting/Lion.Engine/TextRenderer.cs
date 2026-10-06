using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Controls scene-authored bitmap text using Lion's native sprite-batch renderer.</summary>
public sealed class TextRenderer : Component
{
	/// <summary>Copies the native text style without changing the target's text.</summary>
	public void CopyStyleTo(TextRenderer target) { ArgumentNullException.ThrowIfNull(target); TransformState state = default; NativeApi.Hierarchy(Entity.Handle, 8, ref state, target.Entity.Handle); }
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
