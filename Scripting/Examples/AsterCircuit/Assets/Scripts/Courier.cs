using System.Numerics;
using Lion.Engine;

namespace AsterCircuit;

public sealed class Courier : Behaviour
{
	internal const int MaxHealth = 24;
	private StageController? _game;
	private RigidBody2D? _body;
	private SpriteRenderer? _sprite;
	private InputAction _left, _right, _up, _down, _jump, _fire, _weapon;
	private float _coyote, _buffer, _shootTimer, _hurtTimer, _time;
	private int _facing = 1, _lastFrame = -1;
	internal int Health { get; private set; } = MaxHealth;
	internal int Energy { get; private set; } = 24;
	internal bool Grounded { get; private set; }
	internal int ShotsFired { get; private set; }
	internal int Jumps { get; private set; }
	internal float HighestY { get; private set; }
	internal float MoveStrength { get; private set; }
	internal int Facing => _facing;
	internal float HorizontalSpeed => _body?.LinearVelocity.X ?? 0;
	internal float VerticalSpeed => _body?.LinearVelocity.Y ?? 0;
	internal void Initialize(StageController game)
	{
		_game = game;
		_body = Entity.GetComponent<RigidBody2D>()!;
		_sprite = Entity.GetComponent<SpriteRenderer>()!;
		_left = Input.Action("MoveLeft"); _right = Input.Action("MoveRight");
		_up = Input.Action("ClimbUp"); _down = Input.Action("ClimbDown"); _jump = Input.Action("Jump");
		_fire = Input.Action("Fire"); _weapon = Input.Action("Weapon");
	}
	public override void OnUpdate(float deltaTime)
	{
		if (_game == null || _body == null || _sprite == null || _game.IsEnding) return;
		_time += deltaTime; _hurtTimer = Math.Max(0, _hurtTimer - deltaTime); _shootTimer = Math.Max(0, _shootTimer - deltaTime);
		Vector2 position = Transform.Position, velocity = _body.LinearVelocity;
		HighestY = Math.Max(HighestY, position.Y);
		Grounded = Entity.Scene.RaycastNormal(position - new Vector2(0, 38), new(0, -15), out var normal, Entity) && normal.Y > 0.55f;
		_coyote = Grounded ? 0.09f : Math.Max(0, _coyote - deltaTime);
		_buffer = _jump.WasPressed ? 0.13f : Math.Max(0, _buffer - deltaTime);
		float direction = _right.Strength - _left.Strength;
		MoveStrength = direction;
		if (Math.Abs(direction) > 0.1f) _facing = direction > 0 ? 1 : -1;
		// Ice conserves horizontal momentum; other stages stop promptly when the control is released.
		float target = direction * 300;
		velocity.X = _game.Stage == 1 ? velocity.X + Math.Clamp(target - velocity.X, -deltaTime * 650, deltaTime * 650) : target;
		if (_game.Stage == 2 && !Grounded) velocity.X += MathF.Sin(_time * 0.8f) * 22;
		float climb = _up.Strength - _down.Strength;
		bool climbing = _game.IsNearLadder(position) && Math.Abs(climb) > 0.1f;
		if (climbing)
		{
			velocity.Y = climb * 230 + deltaTime * 3600;
			if (climb > 0 && _game.TryLadderExit(position, out var exit)) { _body.SetPosition(exit); velocity = Vector2.Zero; }
		}
		else if (_buffer > 0 && _coyote > 0) { velocity.Y = 1320; _buffer = _coyote = 0; Jumps++; _game.PlayEffect("Jump", 0.3f); }
		else if (_jump.Strength <= 0 && velocity.Y > 450) velocity.Y = Math.Max(450, velocity.Y - deltaTime * 3200);
		_body.LinearVelocity = velocity;
		if (_weapon.WasPressed) GameSession.CycleWeapon();
		if (_fire.Strength > 0 && _shootTimer <= 0 && _game.Fire(position + new Vector2(_facing * 35, 5), _facing, GameSession.Weapon, Energy))
		{
			_shootTimer = 0.2f; ShotsFired++;
			if (GameSession.Weapon != 0) Energy = Math.Max(0, Energy - 1);
		}
		Entity.SetVisible(_hurtTimer <= 0 || (int)(_hurtTimer * 18) % 2 == 0);
		int frame = _hurtTimer > 0.8f ? 6 : _shootTimer > 0.1f ? 5 : !Grounded ? 4 : Math.Abs(velocity.X) > 15 ? 1 + (int)(_time * 9) % 3 : 0;
		if (frame != _lastFrame) { Art.Frame(_sprite, 0, frame, new(96)); _lastFrame = frame; }
		_sprite.FlipX = _facing < 0;
		if (position.Y < -160)
		{
			// Falling out of the world cannot be postponed by contact invulnerability.
			Health = 0; _game.PlayerDied(); return;
		}
		if (position.X < 60) _body.SetPosition(new(60, position.Y));
		if (position.X > StageController.Length - 60) _body.SetPosition(new(StageController.Length - 60, position.Y));
		_game.UpdatePlayerHud(Health, Energy);
	}
	internal void Damage(int amount)
	{
		if (_hurtTimer > 0 || Health <= 0 || _game == null || _body == null || _game.IsEnding) return;
		Health = Math.Max(0, Health - amount); _hurtTimer = 1.2f;
		_body.LinearVelocity = new(-_facing * 240, 450);
		_game.PlayEffect("Hit", 0.5f);
		_game.Burst(Transform.Position);
		if (Health == 0) _game.PlayerDied();
	}
	internal void Collect(bool health)
	{
		if (health) Health = Math.Min(MaxHealth, Health + 6); else Energy = Math.Min(24, Energy + 8);
		_game?.PlayEffect("Pickup", 0.5f);
	}
	internal void Restore() { Health = MaxHealth; Energy = 24; }
	public override void OnCollision(Entity other)
	{
		var enemy = other.GetBehaviour<Warden>();
		if (enemy != null && enemy.IsAlive) Damage(enemy.IsBoss ? 3 : 2);
	}
}
