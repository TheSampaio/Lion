using System.Numerics;

namespace Lion.Engine;

/// <summary>The native rigid body's simulation category.</summary>
public enum BodyType
{
	/// <summary>Immovable geometry.</summary>
	Static,
	/// <summary>Velocity-controlled body without forces.</summary>
	Kinematic,
	/// <summary>Force, gravity and contact controlled body.</summary>
	Dynamic
}

/// <summary>A scene's native Box2D body. Velocities and teleport positions are in pixels.</summary>
public sealed class RigidBody2D : Component
{
	internal RigidBody2D(Entity entity) : base(entity, 3) { }
	/// <summary>Reads or writes velocity without managed allocation.</summary>
	public Vector2 LinearVelocity
	{
		get { Span<float> v = stackalloc float[10]; Command(10, v); return new(v[0], v[1]); }
		set { Span<float> v = stackalloc float[10]; v[0] = value.X; v[1] = value.Y; Command(11, v); }
	}
	/// <summary>Sets initial body settings. Call from AddComponent's initializer, before native Awake.</summary>
	public void Configure(BodyType type, bool fixedRotation = false) { Span<float> v = stackalloc float[10]; v[0] = (int)type; v[1] = fixedRotation ? 1 : 0; Command(14, v); }
	/// <summary>Teleports the simulated body and its transform.</summary>
	public void SetPosition(Vector2 position) { Span<float> v = stackalloc float[10]; v[0] = position.X; v[1] = position.Y; Command(12, v); }
	/// <summary>The native configured body type.</summary>
	public BodyType BodyType { get { Span<float> v = stackalloc float[10]; Command(13, v); return (BodyType)(int)v[0]; } }
	/// <summary>Whether the configured body locks rotation.</summary>
	public bool IsFixedRotation { get { Span<float> v = stackalloc float[10]; Command(13, v); return v[1] != 0; } }
}

/// <summary>A native rectangular collider; requires RigidBody2D.</summary>
public sealed class BoxCollider2D : Component
{
	internal BoxCollider2D(Entity entity) : base(entity, 4) { }
	/// <summary>Configures the unscaled pixel rectangle and material before Awake; RefreshShape applies live changes.</summary>
	public void Configure(Vector2 size, float density = 1, float friction = 0.2f, float restitution = 0)
	{
		if (size.X <= 0 || size.Y <= 0 || density < 0 || friction < 0 || restitution < 0) throw new ArgumentOutOfRangeException(nameof(size));
		Span<float> v = stackalloc float[10]; v[0] = size.X; v[1] = size.Y; v[2] = density; v[3] = friction; v[4] = restitution; Command(73, v);
	}
	/// <summary>The stored unscaled dimensions.</summary>
	public Vector2 Size { get { Span<float> v = stackalloc float[10]; Command(72, v); return new(v[0], v[1]); } }
	/// <summary>The configured density.</summary>
	public float Density { get { Span<float> v = stackalloc float[10]; Command(72, v); return v[2]; } }
	/// <summary>The configured friction.</summary>
	public float Friction { get { Span<float> v = stackalloc float[10]; Command(72, v); return v[3]; } }
	/// <summary>The configured restitution.</summary>
	public float Restitution { get { Span<float> v = stackalloc float[10]; Command(72, v); return v[4]; } }
	/// <summary>Recreates the shape from its configuration and current world scale.</summary>
	public void RefreshShape() { Span<float> v = stackalloc float[10]; Command(71, v); }
}

/// <summary>A native circle collider; requires RigidBody2D.</summary>
public sealed class CircleCollider2D : Component
{
	internal CircleCollider2D(Entity entity) : base(entity, 5) { }
	/// <summary>Configures radius in pixels and material; RefreshShape applies live changes.</summary>
	public void Configure(float radius, float density = 1, float friction = 0.2f, float restitution = 0)
	{
		if (radius <= 0 || density < 0 || friction < 0 || restitution < 0) throw new ArgumentOutOfRangeException(nameof(radius));
		Span<float> v = stackalloc float[10]; v[0] = radius; v[1] = density; v[2] = friction; v[3] = restitution; Command(75, v);
	}
	/// <summary>The configured unscaled radius in pixels.</summary>
	public float Radius { get { Span<float> v = stackalloc float[10]; Command(74, v); return v[0]; } }
	/// <summary>The configured density.</summary>
	public float Density { get { Span<float> v = stackalloc float[10]; Command(74, v); return v[1]; } }
	/// <summary>The configured friction.</summary>
	public float Friction { get { Span<float> v = stackalloc float[10]; Command(74, v); return v[2]; } }
	/// <summary>The configured restitution.</summary>
	public float Restitution { get { Span<float> v = stackalloc float[10]; Command(74, v); return v[3]; } }
	/// <summary>Recreates the native shape.</summary>
	public void RefreshShape() { Span<float> v = stackalloc float[10]; Command(71, v); }
}

/// <summary>The native scene camera. Additional limits/smoothing fields use GetField/SetField.</summary>
public sealed class Camera2D : Component
{
	internal Camera2D(Entity entity) : base(entity, 2) { }
	/// <summary>World units per logical screen pixel.</summary>
	public float Zoom { get => GetField<float>("Zoom"); set => SetField("Zoom", Math.Clamp(value, 0.01f, 100)); }
	/// <summary>The resolved world view position.</summary>
	public Vector2 ViewPosition { get { Span<float> v = stackalloc float[10]; Command(70, v); return new(v[0], v[1]); } }
	/// <summary>The logical view dimensions in world pixels.</summary>
	public Vector2 ViewSize { get { Span<float> v = stackalloc float[10]; Command(70, v); return new(v[3], v[4]); } }
}

/// <summary>A native audio source with scene-owned playback lifetime.</summary>
public sealed class AudioPlayer : Component
{
	internal AudioPlayer(Entity entity) : base(entity, 6) { }
	/// <summary>The resource-relative source path.</summary>
	public string ClipPath { get => Internal.NativeApi.ReadText(Entity.Handle, 3); set { if (value.Length != 0) Internal.ResourcePath.Validate(value); Internal.NativeApi.WriteText(Entity.Handle, 3, value); } }
	/// <summary>The source's volume.</summary>
	public float Volume { get { Span<float> v = stackalloc float[10]; Command(35, v); return v[0]; } set { Span<float> v = stackalloc float[10]; v[0] = value; Command(36, v); } }
	/// <summary>The playback pitch multiplier.</summary>
	public float Pitch { get { Span<float> v = stackalloc float[10]; Command(35, v); return v[1]; } set { Span<float> v = stackalloc float[10]; v[0] = value; Command(37, v); } }
	/// <summary>Whether new voices loop.</summary>
	public bool IsLooping { get => GetField<bool>("Loop"); set => SetField("Loop", value); }
	/// <summary>Whether native Awake starts this source.</summary>
	public bool PlayOnAwake { get => GetField<bool>("Play On Awake"); set => SetField("Play On Awake", value); }
	/// <summary>The mixer bus used by new voices.</summary>
	public AudioBus Bus { get { Span<float> v = stackalloc float[10]; Command(33, v); return (AudioBus)(int)v[0]; } set { Span<float> v = stackalloc float[10]; v[0] = (int)value; Command(34, v); } }
	/// <summary>Starts a voice; false means unavailable output or clip.</summary>
	public bool Play() { Span<float> v = stackalloc float[10]; Command(30, v); return v[0] != 0; }
	/// <summary>Stops this source's voices.</summary>
	public void Stop() { Span<float> v = stackalloc float[10]; Command(31, v); }
	/// <summary>Whether at least one source voice is playing.</summary>
	public bool IsPlaying { get { Span<float> v = stackalloc float[10]; Command(32, v); return v[0] != 0; } }
}

/// <summary>The native particle component. Its authored fields share the Inspector's field names.</summary>
public sealed class ParticleEmitter : Component
{
	internal ParticleEmitter(Entity entity) : base(entity, 7) { }
	/// <summary>Sets the emission position used by the next burst.</summary>
	public void SetEmitterPosition(Vector2 position) { Span<float> v = stackalloc float[10]; v[0] = position.X; v[1] = position.Y; Command(41, v); }
	/// <summary>Emits a bounded burst at the emitter's configured position.</summary>
	public void Emit(int count = 1) { Span<float> v = stackalloc float[10]; v[0] = count; Command(40, v); }
	/// <summary>Emits at a world position.</summary>
	public void EmitAt(Vector2 position, int count = 1) { Span<float> v = stackalloc float[10]; v[0] = count; v[1] = position.X; v[2] = position.Y; v[3] = 1; Command(40, v); }
}

/// <summary>Native camera finishing effects; all effect fields are reflected and writable.</summary>
public sealed class PostProcessing : Component
{
	internal PostProcessing(Entity entity) : base(entity, 8) { }
	/// <summary>Black fade amount from zero to one.</summary>
	public float Fade { get => GetField<float>("Fade"); set => SetField("Fade", Math.Clamp(value, 0, 1)); }
}

/// <summary>The native screen-space button.</summary>
public sealed class Button : Component
{
	internal Button(Entity entity) : base(entity, 9) { }
	/// <summary>Copies the native visual style to another button.</summary>
	public void CopyVisualStyleTo(Button target) { ArgumentNullException.ThrowIfNull(target); TransformState state = default; Internal.NativeApi.Hierarchy(Entity.Handle, 9, ref state, target.Entity.Handle); }
	/// <summary>True for the native click frame.</summary>
	public bool WasClicked { get { Span<float> v = stackalloc float[10]; Command(50, v); return v[0] != 0; } }
	/// <summary>Whether the pointer is over the native button.</summary>
	public bool IsHovered { get { Span<float> v = stackalloc float[10]; Command(50, v); return v[1] != 0; } }
	/// <summary>The keyboard/controller selection highlight.</summary>
	public bool IsSelected { get { Span<float> v = stackalloc float[10]; Command(50, v); return v[2] != 0; } set { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; Command(51, v); } }
	/// <summary>Enables pointer interaction.</summary>
	public void SetInteractable(bool value) { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; Command(52, v); }
}

/// <summary>The native screen-space checkbox.</summary>
public sealed class CheckBox : Component
{
	internal CheckBox(Entity entity) : base(entity, 10) { }
	/// <summary>The checkbox state.</summary>
	public bool IsChecked { get { Span<float> v = stackalloc float[10]; Command(53, v); return v[0] != 0; } set { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; Command(54, v); } }
	/// <summary>True for the native change frame.</summary>
	public bool WasChanged { get { Span<float> v = stackalloc float[10]; Command(53, v); return v[1] != 0; } }
	/// <summary>Whether the native pointer is over this checkbox.</summary>
	public bool IsHovered { get { Span<float> v = stackalloc float[10]; Command(53, v); return v[2] != 0; } }
	/// <summary>Enables pointer interaction.</summary>
	public void SetInteractable(bool value) { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; Command(52, v); }
}

/// <summary>The native dropdown; options, prefix and visual fields use the shared reflection API.</summary>
public sealed class ComboBox : Component
{
	internal ComboBox(Entity entity) : base(entity, 11) { }
	/// <summary>Changes the native prefix and immediately refreshes its label.</summary>
	public void SetPrefix(string prefix) => Internal.NativeApi.WriteText(Entity.Handle, 4, prefix);
	/// <summary>The selected native option.</summary>
	public int SelectedIndex { get { Span<float> v = stackalloc float[10]; Command(55, v); return (int)v[0]; } set { Span<float> v = stackalloc float[10]; v[0] = value; Command(56, v); } }
	/// <summary>True for the native change frame.</summary>
	public bool WasChanged { get { Span<float> v = stackalloc float[10]; Command(55, v); return v[1] != 0; } }
	/// <summary>Whether the popup is open.</summary>
	public bool IsOpen { get { Span<float> v = stackalloc float[10]; Command(55, v); return v[2] != 0; } set { Span<float> v = stackalloc float[10]; v[0] = value ? 1 : 0; Command(57, v); } }
	/// <summary>Selects the next or previous option.</summary>
	public void SelectRelative(int direction) { Span<float> v = stackalloc float[10]; v[0] = direction; Command(58, v); }
}

/// <summary>The native display or interactive value bar.</summary>
public sealed class ProgressBar : Component
{
	internal ProgressBar(Entity entity) : base(entity, 12) { }
	/// <summary>The native value, clamped by the stored range.</summary>
	public float Value { get { Span<float> v = stackalloc float[10]; Command(59, v); return v[0]; } set { Span<float> v = stackalloc float[10]; v[0] = value; Command(60, v); } }
	/// <summary>True for the native pointer change frame.</summary>
	public bool WasChanged { get { Span<float> v = stackalloc float[10]; Command(59, v); return v[1] != 0; } }
	/// <summary>Sets the native minimum and maximum.</summary>
	public void SetRange(float minimum, float maximum) { Span<float> v = stackalloc float[10]; v[0] = minimum; v[1] = maximum; Command(61, v); }
}

/// <summary>The native normalized screen anchor; Anchor and Offset are Vector3 reflected fields.</summary>
public sealed class WidgetAnchor : Component
{
	internal WidgetAnchor(Entity entity) : base(entity, 13) { }
}
