using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A typed, non-owning view of a native trait on an entity.</summary>
/// <remarks>Engine-thread-only. Each operation revalidates the owner and resolves the trait; no
/// native pointer is retained. If the editor replaces that trait, this view addresses its replacement.
/// Cache views obtained from Entity.GetComponent outside update loops.</remarks>
public abstract class Component
{
	private protected Component(Entity entity, int kind) { Entity = entity; Kind = kind; }
	internal int Kind { get; }

	/// <summary>The owner whose lifetime governs this view.</summary>
	public Entity Entity { get; }

	/// <summary>Whether the owner still exists and currently carries this native trait.</summary>
	public bool IsValid => Entity.IsValid && NativeApi.HasComponent(Entity.Handle, Kind);

	/// <summary>Whether the native component participates in its owner's callbacks and rendering.</summary>
	public bool IsEnabled
	{
		get => NativeApi.GetComponentState(Entity.Handle, Kind, 0) != 0;
		set => NativeApi.SetComponentState(Entity.Handle, Kind, 0, value ? 1 : 0);
	}
}

internal static class ComponentType<T> where T : Component
{
	internal static readonly int Kind = typeof(T) == typeof(TextRenderer) ? 0
		: typeof(T) == typeof(SpriteRenderer) ? 1 : throw new NotSupportedException($"{typeof(T).Name} has no native binding.");
	internal static T Create(Entity entity) => Kind == 0 ? (T)(Component)new TextRenderer(entity) : (T)(Component)new SpriteRenderer(entity);
}
