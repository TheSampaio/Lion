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
	}

	public override void OnDestroy() => Log.Info("[EditorProbe] Destroy");
}
