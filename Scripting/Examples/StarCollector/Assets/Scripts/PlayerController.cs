using System.Numerics;
using Lion.Engine;

namespace StarCollector;

public sealed class PlayerController : Behaviour
{
	[Editable] public float Speed = 320;
	private InputAction _horizontal;
	private InputAction _vertical;
	private SpriteRenderer _sprite = null!;

	public override void OnAwake()
	{
		_horizontal = Input.Action("MoveX");
		_vertical = Input.Action("MoveY");
		_sprite = Entity.GetComponent<SpriteRenderer>() ?? throw new InvalidOperationException("Player requires a Sprite Renderer.");
	}

	public override void OnUpdate(float deltaTime)
	{
		var direction = new Vector2(_horizontal.Strength, _vertical.Strength);
		if (direction.LengthSquared() > 1) { direction = Vector2.Normalize(direction); }
		var position = Transform.Position + direction * Speed * deltaTime;
		Transform.Position = Vector2.Clamp(position, new Vector2(-520, -240), new Vector2(520, 240));
		if (direction.X != 0) { _sprite.FlipX = direction.X < 0; }
	}
}
