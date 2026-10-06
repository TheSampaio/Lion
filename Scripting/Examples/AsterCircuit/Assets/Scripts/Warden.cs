using System.Numerics;
using Lion.Engine;

namespace AsterCircuit;

internal enum EnemyKind { Crawler, Drone, Turret, Boss }

public sealed class Warden : Behaviour
{
	private StageController? _game;
	private RigidBody2D? _body;
	private SpriteRenderer? _sprite;
	private EnemyKind _kind;
	private Vector2 _home;
	private float _time, _attack, _damageCooldown;
	private int _direction = -1, _frame = -1;
	internal int Health { get; private set; }
	internal bool IsBoss => _kind == EnemyKind.Boss;
	internal bool IsAlive => Health > 0;
	internal bool IsShielded => IsBoss && _game?.Stage == 1 && _time % 3.8f < 1.1f;
	internal void Initialize(StageController game, EnemyKind kind)
	{
		_game = game; _kind = kind; _home = Transform.Position;
		_body = Entity.GetComponent<RigidBody2D>()!; _sprite = Entity.GetComponent<SpriteRenderer>()!;
		Health = kind == EnemyKind.Boss ? 36 : kind == EnemyKind.Turret ? 5 : 3;
		_attack = kind == EnemyKind.Boss ? 1.2f : 1.8f;
	}
	public override void OnUpdate(float deltaTime)
	{
		if (_game == null || _body == null || _sprite == null || !IsAlive || _game.IsEnding) return;
		if (IsBoss && !_game.BossActive) return;
		Vector2 player = _game.PlayerPosition, position = Transform.Position;
		if (!IsBoss && Math.Abs(player.X - position.X) > 950) return;
		_time += deltaTime; _attack -= deltaTime; _damageCooldown = Math.Max(0, _damageCooldown - deltaTime);
		Vector2 velocity = _body.LinearVelocity;
		if (_kind == EnemyKind.Crawler)
		{
			if (position.X < _home.X - 135) _direction = 1;
			if (position.X > _home.X + 135) _direction = -1;
			velocity.X = _direction * 90;
			_body.LinearVelocity = velocity;
		}
		else if (_kind == EnemyKind.Drone)
			_body.SetPosition(_home + new Vector2(MathF.Sin(_time * 1.5f) * 125, MathF.Sin(_time * 2) * 60));
		else if (IsBoss)
		{
			if (_game.Stage == 0)
			{
				velocity.X = _time % 4 > 3 ? Math.Sign(player.X - position.X) * 250 : 0;
				if (_time % 4 > 3 && Entity.Scene.RaycastNormal(position - new Vector2(0, 62), new(0, -15), out var normal, Entity) && normal.Y > 0.55f) velocity.Y = 950;
				_body.LinearVelocity = velocity;
			}
			else if (_game.Stage == 1) _body.SetPosition(_home + new Vector2(MathF.Sin(_time * 0.8f) * 160, MathF.Sin(_time * 1.5f) * 38));
			else _body.SetPosition(_home + new Vector2(MathF.Sin(_time * 1.4f) * 240, 95 + MathF.Sin(_time * 2.2f) * 110));
		}
		if (_attack <= 0)
		{
			_attack = IsBoss ? (_game.Stage == 2 ? 0.85f : 1.4f) : 2.1f;
			var direction = player - position;
			if (direction.LengthSquared() < 1) direction = -Vector2.UnitX;
			direction = Vector2.Normalize(direction);
			if (IsBoss)
			{
				float spread = _game.Stage == 0 ? 0.26f : _game.Stage == 1 ? 0.13f : 0.4f;
				for (int offset = -1; offset <= 1; offset++) _game.EnemyFire(position, Rotate(direction, offset * spread) * (_game.Stage == 2 ? 440 : 350), _game.Stage, Entity);
			}
			else if (_kind != EnemyKind.Crawler) _game.EnemyFire(position, direction * 330, _game.Stage, Entity);
		}
		int frame = IsBoss ? IsShielded ? 4 : (_attack < 0.3f ? 3 : (int)(_time * 4) % 2) : _kind == EnemyKind.Crawler ? (int)(_time * 5) % 2 : _kind == EnemyKind.Drone ? 2 + (int)(_time * 7) % 2 : 4;
		if (frame != _frame) { Art.Frame(_sprite, IsBoss ? _game.Stage + 2 : 1, frame, new(IsBoss ? 160 : 78)); _frame = frame; }
		_sprite.FlipX = player.X > position.X;
		_sprite.SetColor(_damageCooldown > 0 ? new(1, 0.35f, 0.4f) : IsShielded ? new(0.5f, 0.8f, 1) : Vector3.One);
	}
	internal void Hit(int weapon)
	{
		if (_game == null || !IsAlive || IsBoss && !_game.BossActive || _damageCooldown > 0 || IsShielded) return;
		int damage = 1;
		int weakness = _game.Stage == 0 ? 2 : _game.Stage == 1 ? 3 : 1;
		if (weapon != 0) damage = IsBoss && weapon == weakness ? 4 : 2;
		Health = Math.Max(0, Health - damage); _damageCooldown = 0.11f;
		_game.Burst(Transform.Position);
		_game.PlayEffect("Hit", 0.18f);
		if (Health == 0)
		{
			Entity.SetEnabled(false);
			if (IsBoss) _game.BossDefeated(); else _game.EnemyDefeated(Transform.Position);
		}
	}
	private static Vector2 Rotate(Vector2 vector, float angle) => new(vector.X * MathF.Cos(angle) - vector.Y * MathF.Sin(angle), vector.X * MathF.Sin(angle) + vector.Y * MathF.Cos(angle));
}
