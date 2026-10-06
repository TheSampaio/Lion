using System.Numerics;
using Lion.Engine;

namespace AsterCircuit;

internal static class Art
{
	internal const string Atlas = "Sprites/CircuitAtlas.png";
	internal static readonly Vector3 Amber = new(1, 0.64f, 0.25f);
	internal static readonly Vector3 Ice = new(0.52f, 0.75f, 1);
	internal static readonly Vector3 Mint = new(0.3f, 1, 0.7f);
	internal static readonly string[] StageNames = ["PYRE FOUNDRY", "GLACIER VAULT", "TEMPEST ARRAY"];
	internal static readonly string[] BossNames = ["CRUCIBLE", "RIME WARDEN", "VOLT KESTREL"];
	internal static readonly string[] Tracks = ["Forge", "Frost", "Storm"];
	internal static readonly Vector3[] Colors = [Amber, Ice, Mint];

	internal static SpriteRenderer Sprite(Scene scene, string name, Vector2 position, int row, int column, Vector2 size, int order = 0)
	{
		var entity = scene.CreateEntity(name);
		entity.Transform.Position = position;
		return entity.AddComponent<SpriteRenderer>(sprite => { sprite.Texture = TextureAsset.FromPath(Atlas); sprite.Order = order; Frame(sprite, row, column, size); });
	}
	internal static void Frame(SpriteRenderer sprite, int row, int column, Vector2 size)
	{
		// The generated atlas has eight evenly spaced rows; OpenGL samples bottom-up.
		sprite.SetRegion(new(column / 8f, 1 - (row + 1) / 8f), new((column + 1) / 8f, 1 - row / 8f), size);
	}
	internal static SpriteRenderer Rectangle(Scene scene, string name, Vector2 position, Vector2 size, Vector3 color, int order = 80)
	{
		var sprite = Sprite(scene, name, position, 0, 0, size, order);
		// One near-white opaque atlas texel is a neutral primitive, not a separately generated image.
		sprite.SetRegion(new(93f / 1254, 751f / 1254), new(94f / 1254, 752f / 1254), size);
		sprite.SetColor(color);
		return sprite;
	}
	internal static TextRenderer Text(Scene scene, string name, string text, Vector2 position, float size, Vector3? color = null)
	{
		var entity = scene.CreateEntity(name);
		entity.Transform.Position = position;
		return entity.AddComponent<TextRenderer>(label =>
		{
			label.SetAssetField("Font", "Fonts/Arcade.lnfont");
			label.SetField("Size", size);
			label.SetField("Color", color ?? Vector3.One);
			label.SetField("Order", 120);
			label.Text = text;
		});
	}
	internal static AudioPlayer Music(Scene scene, string track)
	{
		return scene.CreateEntity("Music").AddComponent<AudioPlayer>(audio =>
		{
			audio.ClipPath = "Sounds/" + track + ".wav";
			audio.IsLooping = true; audio.Volume = 0.65f; audio.Bus = AudioBus.Music;
		});
	}
	internal static void Terrain(Scene scene, Vector2 position, int theme, bool solid = true, int order = 0)
	{
		var entity = Sprite(scene, solid ? "Terrain" : "Backdrop", position, 5, theme * 2, new(48), order).Entity;
		if (!solid) { entity.GetComponent<SpriteRenderer>()!.SetColor(new(0.17f, 0.2f, 0.27f)); return; }
	}
	internal static void Collider(Scene scene, Vector2 position, Vector2 size, int theme)
	{
		var entity = scene.CreateEntity("Solid Span"); entity.Transform.Position = position;
		entity.AddComponent<RigidBody2D>(body => body.Configure(BodyType.Static));
		entity.AddComponent<BoxCollider2D>(box => box.Configure(size, friction: theme == 1 ? 0.015f : 0.2f));
	}
}

internal sealed class HudBar
{
	private readonly ProgressBar _state;
	private readonly SpriteRenderer _fill;
	private readonly Vector2 _size;
	internal HudBar(Scene scene, Entity camera, string name, Vector2 position, Vector2 size, float maximum, Vector3 color)
	{
		_size = size;
		var frame = Art.Rectangle(scene, name + " Frame", position, size + new Vector2(6), new(0.13f, 0.17f, 0.25f), 99);
		frame.Entity.SetParent(camera, false);
		_fill = Art.Rectangle(scene, name + " Fill", position, size, color, 100);
		_fill.Entity.SetParent(camera, false);
		_state = frame.Entity.AddComponent<ProgressBar>(bar => { bar.SetRange(0, maximum); bar.Value = maximum; });
	}
	internal void Set(float value, float maximum)
	{
		_state.Value = value;
		float ratio = Math.Clamp(_state.Value / maximum, 0.001f, 1);
		var transform = _fill.Entity.Transform.State;
		transform.Scale = new(ratio, 1);
		transform.Position.X = _state.Entity.Transform.Position.X - _size.X * (1 - ratio) / 2;
		_fill.Entity.Transform.State = transform;
	}
}
