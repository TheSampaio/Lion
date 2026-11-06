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

	/// <summary>Reads a field by the exact name shown in the Inspector. Intended for configuration, not hot loops.</summary>
	public T GetField<T>(string name) => NativeApi.Field(Entity.Handle, Kind, name, default(T)!, false);
	/// <summary>Writes a native reflected field. Supported types: float, int, bool, string, Vector2, Vector3.</summary>
	public void SetField<T>(string name, T value) => NativeApi.Field(Entity.Handle, Kind, name, value, true);
	/// <summary>Writes an asset field using a portable resource-relative identity.</summary>
	public void SetAssetField(string name, string path) { if (path.Length != 0) ResourcePath.Validate(path); SetField(name, path); }
	private protected void Command(int operation, Span<float> values) => NativeApi.Command(Entity.Handle, Kind, operation, values);
}

internal static class ComponentType<T> where T : Component
{
	internal static readonly int Kind = typeof(T) == typeof(TextRenderer) ? 0 : typeof(T) == typeof(SpriteRenderer) ? 1
		: typeof(T) == typeof(Camera2D) ? 2 : typeof(T) == typeof(RigidBody2D) ? 3 : typeof(T) == typeof(BoxCollider2D) ? 4
		: typeof(T) == typeof(CircleCollider2D) ? 5 : typeof(T) == typeof(AudioPlayer) ? 6 : typeof(T) == typeof(ParticleEmitter) ? 7
		: typeof(T) == typeof(PostProcessing) ? 8 : typeof(T) == typeof(Button) ? 9 : typeof(T) == typeof(CheckBox) ? 10
		: typeof(T) == typeof(ComboBox) ? 11 : typeof(T) == typeof(ProgressBar) ? 12 : typeof(T) == typeof(WidgetAnchor) ? 13
		: throw new NotSupportedException($"{typeof(T).Name} has no native binding.");
	internal static T Create(Entity entity) => (T)(Component)(Kind switch
	{
		0 => new TextRenderer(entity), 1 => new SpriteRenderer(entity), 2 => new Camera2D(entity), 3 => new RigidBody2D(entity),
		4 => new BoxCollider2D(entity), 5 => new CircleCollider2D(entity), 6 => new AudioPlayer(entity), 7 => new ParticleEmitter(entity),
		8 => new PostProcessing(entity), 9 => new Button(entity), 10 => new CheckBox(entity), 11 => new ComboBox(entity),
		12 => new ProgressBar(entity), 13 => new WidgetAnchor(entity), _ => throw new NotSupportedException()
	});
}
