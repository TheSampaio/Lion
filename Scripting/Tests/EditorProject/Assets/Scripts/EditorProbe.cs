using System.Numerics;
using Lion.Engine;

namespace Game;

public sealed class EditorProbe : Behaviour
{
	[Editable] public float Speed = 120;
	[Editable] public int Lives = 3;
	[Editable] public bool Moving = true;
	[Editable] public string Caption = "C# authoring";
	[Editable] public Vector2 Direction = Vector2.UnitX;
	private float _elapsed;

	public override void OnAwake()
	{
		_elapsed = 0;
		Log.Info("[EditorProbe] Awake: " + Caption);
	}

	public override void OnUpdate(float deltaTime)
	{
		_elapsed += deltaTime;
		if (Moving)
		{
			Transform.Position = Direction * (MathF.Sin(_elapsed) * Speed);
		}
		string? report = Environment.GetEnvironmentVariable("LION_SCRIPT_TEST_REPORT");
		if (report != null && _elapsed >= 0.15f)
		{
			if (!Entity.HasComponent<SpriteRenderer>() || Entity.Scene.FindEntity("C# Mover") == null)
			{
				throw new InvalidOperationException("Packaged gameplay bindings failed.");
			}
			File.WriteAllText(report, System.Text.Json.JsonSerializer.Serialize(new
			{
				Position = Transform.Position.X,
				Framework = typeof(object).Assembly.Location,
				Script = GetType().Assembly.Location
			}));
			Application.RequestQuit();
		}
	}

	public override void OnDestroy() => Log.Info("[EditorProbe] Destroy");
}
