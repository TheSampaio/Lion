using System.Numerics;
using System.Text.Json;
using Lion.Engine;

namespace AsterCircuit;

public sealed class StageController : Behaviour
{
	internal const float Length = 6144;
	private static readonly (int First, int Last)[][] GapColumns =
	[
		[(24, 26), (58, 60), (89, 91)],
		[(29, 31), (71, 73)],
		[(19, 22), (45, 48), (79, 82)]
	];
	[Editable] private int _stage = 0;
	private readonly List<Warden> _enemies = new(12);
	private readonly List<(Vector2 Position, bool Health, Entity Entity)> _pickups = new(32);
	private readonly List<Vector2> _ladders = [];
	private readonly List<Entity> _checkpoints = [];
	private readonly Bolt[] _bolts = new Bolt[64];
	private readonly List<(SpriteRenderer Sprite, float Time)> _explosions = new(12);
	private Courier? _player;
	private Warden? _boss;
	private Entity _camera, _gate;
	private AudioPlayer? _music, _shot, _jump, _hit, _pickup, _clear;
	private ParticleEmitter? _particles;
	private HudBar? _healthBar, _energyBar, _bossBar;
	private TextRenderer? _status, _pauseText;
	private InputAction _pause, _back;
	private float _ending, _elapsed, _reportTimer;
	private int _endState, _kills, _lastWeapon = -1;
	private string? _report;
	private sealed class Bolt
	{
		internal required SpriteRenderer Sprite;
		internal Vector2 Velocity;
		internal float Time;
		internal bool Hostile;
		internal int Weapon;
		internal Entity Source;
	}
	internal int Stage => _stage;
	internal bool IsEnding => _endState != 0;
	internal bool BossActive { get; private set; }
	internal Vector2 PlayerPosition => _player?.Transform.Position ?? Vector2.Zero;
	public override bool UpdatesWhenPaused => true;

	public override void OnAwake()
	{
		if (_stage is < 0 or > 2) throw new InvalidOperationException("Invalid authored stage.");
		var scene = Entity.Scene;
		_report = Environment.GetEnvironmentVariable("ASTER_TEST_REPORT");
		GameSession.CurrentStage = _stage;
		scene.Gravity = new(0, -36);
		_camera = scene.FindEntity("Camera") ?? throw new InvalidOperationException("The stage needs a Camera entity.");
		_pause = Input.Action("Pause"); _back = Input.Action("Back");
		BuildWorld(scene);
		BuildActors(scene);
		BuildHud(scene);
		_music = Art.Music(scene, Art.Tracks[_stage]);
		_shot = Effect(scene, "Shot"); _jump = Effect(scene, "Jump"); _hit = Effect(scene, "Hit"); _pickup = Effect(scene, "Pickup"); _clear = Effect(scene, "Clear");
		_particles = scene.CreateEntity("Impact Sparks").AddComponent<ParticleEmitter>(particles =>
		{
			particles.SetAssetField("Texture", Art.Atlas); particles.SetField("Max Particles", 64);
			particles.SetField("Lifetime", 0.22f); particles.SetField("Start Size", 12f); particles.SetField("End Size", 2f);
			particles.SetField("Speed", 200f); particles.SetField("Start Color", Art.Colors[_stage]);
		});
		for (int i = 0; i < _bolts.Length; i++)
		{
			var sprite = Art.Sprite(scene, "Pooled Bolt " + i, Vector2.Zero, 6, 4, new(30), 35);
			sprite.Entity.SetEnabled(false);
			_bolts[i] = new Bolt { Sprite = sprite };
		}
		for (int i = 0; i < 12; i++)
		{
			var sprite = Art.Sprite(scene, "Pooled Impact " + i, Vector2.Zero, 7, 0, new(96), 45);
			sprite.Entity.SetEnabled(false); _explosions.Add((sprite, 0));
		}
	}
	private void BuildWorld(Scene scene)
	{
		Art.Rectangle(scene, "Sky", new(Length / 2, 360), new(Length, 900), _stage == 0 ? new(0.10f, 0.025f, 0.04f) : _stage == 1 ? new(0.035f, 0.07f, 0.14f) : new(0.025f, 0.085f, 0.09f), -100);
		int spanStart = 0;
		for (int column = 0; column < 128; column++)
		{
			float x = column * 48 + 24;
			// Authored theme-specific gaps remain narrower than a full held jump.
			bool gap = false;
			foreach (var range in GapColumns[_stage]) if (column >= range.First && column <= range.Last) { gap = true; break; }
			// A continuous floor owns one shape: per-tile boxes snag moving bodies on internal seams.
			if (gap)
			{
				if (column > spanStart) Art.Collider(scene, new((spanStart + column) * 24, 48), new((column - spanStart) * 48, 96), _stage);
				spanStart = column + 1;
			}
			if (!gap) { Art.Terrain(scene, new(x, 24), _stage); Art.Terrain(scene, new(x, 72), _stage); }
			if (column % 3 == 0)
				for (int row = 3; row < 13; row++) Art.Terrain(scene, new(x, row * 48 + 24), _stage, false, -50);
		}
		Art.Collider(scene, new((spanStart + 128) * 24, 48), new((128 - spanStart) * 48, 96), _stage);
		for (int section = 0; section < 5; section++)
		{
			float x = 560 + section * 820;
			// Storm gaps are crossed in the air, so their landing corridor must have no low ceiling.
			if (_stage == 2)
				foreach (var gap in GapColumns[_stage])
					if (x - 24 < (gap.Last + 1) * 48 && x + 168 > gap.First * 48) x = (gap.Last + 1) * 48 + 72;
			int height = _stage == 1 ? 4 : _stage == 2 ? 3 : 2;
			for (int tile = 0; tile < 4; tile++) Art.Terrain(scene, new(x + tile * 48, 96 + height * 48), _stage);
			Art.Collider(scene, new(x + 72, 96 + height * 48), new(192, 48), _stage);
			if (_stage == 1)
			{
				_ladders.Add(new(x - 60, 192));
				for (int rung = 0; rung < 5; rung++) Art.Sprite(scene, "Ladder", new(x - 60, 120 + rung * 48), 5, 6, new(48), 3);
			}
			if (_stage == 0 && section % 2 == 0)
				Art.Sprite(scene, "Heat Vent", new(x + 240, 120), 6, 6, new(60), 5);
		}
		foreach (float x in new[] { 2080f, 4110f })
		{
			var checkpoint = scene.Instantiate("Assemblies/Checkpoint.lnassembly");
			if (!checkpoint.IsValid) throw new InvalidOperationException("Checkpoint Assembly could not be loaded.");
			checkpoint.Transform.Position = new(x, 140);
			Art.Frame(checkpoint.GetComponent<SpriteRenderer>()!, 6, 2, new(84));
			_checkpoints.Add(checkpoint);
		}
		_gate = Art.Sprite(scene, "Arena Gate", new(5232, 288), 5, 7, new(48, 384), 20).Entity;
		_gate.IsEnabled = false;
		_gate.AddComponent<RigidBody2D>(body => body.Configure(BodyType.Static));
		_gate.AddComponent<BoxCollider2D>(box => box.Configure(new(48, 384)));
	}
	private void BuildActors(Scene scene)
	{
		var courier = Art.Sprite(scene, "Courier", new(GameSession.Checkpoint, 155), 0, 0, new(96), 30).Entity;
		courier.AddComponent<RigidBody2D>(body => body.Configure(BodyType.Dynamic, true));
		courier.AddComponent<BoxCollider2D>(box => box.Configure(new(54, 78), friction: 0));
		_player = courier.AddBehaviour<Courier>(); _player.Initialize(this);
		float[] placements = [900, 1500, 1850, 2400, 3000, 3250, 3650, 4400, 4750, 4950];
		for (int i = 0; i < placements.Length; i++)
		{
			var kind = i % 3 == 0 ? EnemyKind.Drone : i % 3 == 1 ? EnemyKind.Crawler : EnemyKind.Turret;
			float x = placements[i];
			// Drones patrol above the ground; turret and crawler lanes avoid authored gaps.
			float y = kind == EnemyKind.Drone ? 270 : kind == EnemyKind.Crawler ? 480 : 150;
			var enemy = CreateEnemy(scene, kind, new(x, y));
			_enemies.Add(enemy);
		}
		_boss = CreateEnemy(scene, EnemyKind.Boss, new(5800, 165));
		_enemies.Add(_boss);
		for (int i = 0; i < 5; i++) DropPickup(new(480 + i * 900, 150), i % 2 == 0);
	}
	private Warden CreateEnemy(Scene scene, EnemyKind kind, Vector2 position)
	{
		bool boss = kind == EnemyKind.Boss;
		var entity = Art.Sprite(scene, boss ? Art.BossNames[_stage] : "Hostile " + kind, position, boss ? _stage + 2 : 1, 0, new(boss ? 160 : 78), 25).Entity;
		entity.AddComponent<RigidBody2D>(body => body.Configure(kind == EnemyKind.Crawler || boss && _stage == 0 ? BodyType.Dynamic : BodyType.Kinematic, true));
		entity.AddComponent<BoxCollider2D>(box => box.Configure(boss ? new(90, 126) : new(48, 48), friction: 0));
		var enemy = entity.AddBehaviour<Warden>(); enemy.Initialize(this, kind); return enemy;
	}
	private void BuildHud(Scene scene)
	{
		_healthBar = new(scene, _camera, "Health", new(-430, 285), new(220, 18), Courier.MaxHealth, Art.Amber);
		_energyBar = new(scene, _camera, "Energy", new(-430, 250), new(220, 12), 24, Art.Ice);
		_bossBar = new(scene, _camera, "Boss Health", new(370, 285), new(250, 18), 36, Art.Colors[_stage]);
		_status = Art.Text(scene, "Status", "", new(0, 320), 18, Art.Colors[_stage]);
		_status.Entity.SetParent(_camera, false);
		_pauseText = Art.Text(scene, "Pause", "PAUSED\nP TO RESUME    ESC TO STAGE SELECT", Vector2.Zero, 24, Art.Amber);
		_pauseText.Entity.SetParent(_camera, false); _pauseText.Entity.SetEnabled(false);
		var title = Art.Text(scene, "Stage Name", Art.StageNames[_stage], new(340, 250), 18, Art.Colors[_stage]); title.Entity.SetParent(_camera, false);
	}
	public override void OnUpdate(float deltaTime)
	{
		var scene = Entity.Scene;
		if (_back.WasPressed) { scene.IsPaused = false; scene.Load("Scenes/Select.lnscene"); return; }
		if (_pause.WasPressed && !IsEnding) { scene.IsPaused = !scene.IsPaused; if (_pauseText != null) _pauseText.Entity.SetEnabled(scene.IsPaused); }
		Report(deltaTime);
		if (IsEnding)
		{
			_ending -= deltaTime;
			if (_ending <= 0)
			{
				scene.IsPaused = false;
				if (_endState == 2) scene.Load(GameSession.Cleared == 7 ? "Scenes/Ending.lnscene" : "Scenes/Select.lnscene");
				else scene.Load(GameSession.Lives > 0 ? GameSession.StagePath(_stage) : "Scenes/GameOver.lnscene");
			}
			return;
		}
		if (scene.IsPaused) return;
		_elapsed += deltaTime;
		Vector2 player = PlayerPosition;
		_camera.Transform.Position = new(Math.Clamp(player.X, 640, Length - 640), 360);
		if (!BossActive && player.X > 5330)
		{
			BossActive = true; _gate.IsEnabled = true;
			_music?.Stop(); if (_music != null) { _music.ClipPath = "Sounds/Boss.wav"; _music.Play(); }
		}
		foreach (var checkpoint in _checkpoints)
			if (player.X > checkpoint.Transform.Position.X && GameSession.Checkpoint < checkpoint.Transform.Position.X)
			{
				GameSession.Checkpoint = checkpoint.Transform.Position.X;
				_player!.Restore();
				checkpoint.GetComponent<SpriteRenderer>()!.SetColor(Art.Mint); PlayEffect("Pickup", 0.5f);
			}
		for (int i = _pickups.Count - 1; i >= 0; i--)
		{
			var pickup = _pickups[i];
			if (Vector2.DistanceSquared(pickup.Position, player) < 80 * 80) { _player!.Collect(pickup.Health); pickup.Entity.Destroy(); _pickups.RemoveAt(i); }
		}
		if (_stage == 0)
			for (int section = 0; section < 5; section += 2)
				if (Math.Abs(player.X - (800 + section * 820)) < 36 && player.Y < 170 && _elapsed % 3.5f < 1.1f) _player!.Damage(2);
		UpdateBolts(deltaTime);
		UpdateExplosions(deltaTime);
		_bossBar?.Set(BossActive ? _boss!.Health : 0, 36);
		if (_lastWeapon != GameSession.Weapon && _status != null)
		{
			_lastWeapon = GameSession.Weapon;
			_status.Text = $"LIVES {GameSession.Lives}    {new[] { "PULSE", "EMBER FAN", "RIME LANCE", "ARC WAVE" }[_lastWeapon]}";
		}
	}
	private void UpdateBolts(float deltaTime)
	{
		foreach (var bolt in _bolts)
		{
			if (bolt.Time <= 0) continue;
			Vector2 oldPosition = bolt.Sprite.Entity.Transform.Position, movement = bolt.Velocity * deltaTime;
			Vector2 position = oldPosition + movement;
			bolt.Time -= deltaTime;
			if (bolt.Time <= 0 || position.Y < -100 || position.Y > 800) { Deactivate(bolt); continue; }
			bolt.Sprite.Entity.Transform.Position = position;
			if (bolt.Hostile)
			{
				if (Near(position, PlayerPosition, new(37, 46))) { _player!.Damage(2); Deactivate(bolt); }
			}
			else
			{
				foreach (var enemy in _enemies)
				{
					if (!enemy.IsAlive || enemy.IsBoss && !BossActive) continue;
					if (Near(position, enemy.Transform.Position, enemy.IsBoss ? new(62, 76) : new(38, 38))) { enemy.Hit(bolt.Weapon); Deactivate(bolt); break; }
				}
			}
			if (bolt.Time > 0 && Entity.Scene.Raycast(oldPosition, movement, out var hit, bolt.Source))
			{
				if (bolt.Hostile && hit.Entity == _player!.Entity) _player.Damage(2);
				else if (!bolt.Hostile) hit.Entity.GetBehaviour<Warden>()?.Hit(bolt.Weapon);
				Deactivate(bolt);
			}
		}
	}
	private void UpdateExplosions(float deltaTime)
	{
		for (int i = 0; i < _explosions.Count; i++)
		{
			var effect = _explosions[i]; if (effect.Time <= 0) continue;
			float time = effect.Time - deltaTime;
			if (time <= 0) effect.Sprite.Entity.SetEnabled(false);
			else Art.Frame(effect.Sprite, 7, Math.Clamp((int)((0.4f - time) * 20), 0, 7), new(96));
			_explosions[i] = (effect.Sprite, time);
		}
	}
	internal bool Fire(Vector2 position, int facing, int weapon, int energy)
	{
		if (weapon != 0 && energy <= 0) weapon = 0;
		int count = weapon == 1 ? 3 : 1;
		bool fired = false;
		for (int i = 0; i < count; i++) fired |= SpawnBolt(position, new(facing * (weapon == 2 ? 1100 : 850), weapon == 1 ? (i - 1) * 160 : 0), false, weapon, _player!.Entity);
		if (fired) PlayEffect("Shot", 0.18f);
		return fired;
	}
	internal void EnemyFire(Vector2 position, Vector2 velocity, int theme, Entity source) => SpawnBolt(position, velocity, true, theme, source);
	private bool SpawnBolt(Vector2 position, Vector2 velocity, bool hostile, int weapon, Entity source)
	{
		foreach (var bolt in _bolts)
		{
			if (bolt.Time > 0) continue;
			bolt.Time = 2.5f; bolt.Velocity = velocity; bolt.Hostile = hostile; bolt.Weapon = weapon;
			bolt.Source = source;
			bolt.Sprite.Entity.Transform.Position = position; bolt.Sprite.Entity.SetEnabled(true);
			Art.Frame(bolt.Sprite, 6, hostile ? weapon == 1 ? 7 : 5 : weapon == 1 ? 6 : weapon == 2 ? 7 : 4, new(hostile ? 34 : 30));
			bolt.Sprite.SetColor(hostile ? Art.Colors[_stage] : weapon == 0 ? Vector3.One : Art.Colors[weapon - 1]);
			bolt.Sprite.FlipX = velocity.X < 0;
			return true;
		}
		return false;
	}
	private static void Deactivate(Bolt bolt) { bolt.Time = 0; bolt.Sprite.Entity.SetEnabled(false); }
	private static bool Near(Vector2 a, Vector2 b, Vector2 half) => Math.Abs(a.X - b.X) < half.X && Math.Abs(a.Y - b.Y) < half.Y;
	internal bool IsNearLadder(Vector2 position)
	{
		foreach (var ladder in _ladders) if (Math.Abs(position.X - ladder.X) < 36 && position.Y > 100 && position.Y < 360) return true;
		return false;
	}
	internal bool TryLadderExit(Vector2 position, out Vector2 destination)
	{
		foreach (var ladder in _ladders)
			if (Math.Abs(position.X - ladder.X) < 36 && position.Y >= 337 && position.Y < 380)
			{
				// A solid platform needs an explicit top dismount, not a climb through its underside.
				destination = new(ladder.X + 90, 352);
				return true;
			}
		destination = default;
		return false;
	}
	internal void UpdatePlayerHud(int health, int energy) { _healthBar?.Set(health, Courier.MaxHealth); _energyBar?.Set(energy, 24); }
	internal void EnemyDefeated(Vector2 position) { _kills++; DropPickup(position, _kills % 2 == 0); }
	private void DropPickup(Vector2 position, bool health)
	{
		if (_pickups.Count >= 32) return;
		var sprite = Art.Sprite(Entity.Scene, "Pickup", position, 6, health ? 1 : 0, new(48), 10);
		_pickups.Add((position, health, sprite.Entity));
	}
	internal void Burst(Vector2 position)
	{
		_particles?.EmitAt(position, 4);
		for (int i = 0; i < _explosions.Count; i++)
		{
			if (_explosions[i].Time > 0) continue;
			var sprite = _explosions[i].Sprite; sprite.Entity.Transform.Position = position; sprite.Entity.SetEnabled(true);
			_explosions[i] = (sprite, 0.4f); break;
		}
	}
	internal void BossDefeated()
	{
		if (IsEnding) return;
		_endState = 2; _ending = 2.5f;
		GameSession.Cleared |= 1 << _stage; GameSession.Checkpoint = 192; GameSession.Save();
		_music?.Stop(); PlayEffect("Clear", 0.7f); Entity.Scene.SetPaused(true);
		_pauseText!.Text = "SIGNAL RESTORED\nNEW WEAPON UNLOCKED"; _pauseText.Entity.SetEnabled(true);
	}
	internal void PlayerDied()
	{
		if (IsEnding) return;
		GameSession.Lives--; _endState = 1; _ending = 1.4f;
		_player!.Entity.SetEnabled(false);
		_pauseText!.Text = "COURIER OFFLINE"; _pauseText.Entity.SetEnabled(true);
	}
	private static AudioPlayer Effect(Scene scene, string name) => scene.CreateEntity(name + " Source").AddComponent<AudioPlayer>(source => { source.ClipPath = "Sounds/" + name + ".wav"; source.PlayOnAwake = false; });
	internal void PlayEffect(string name, float volume)
	{
		var source = name switch { "Shot" => _shot, "Jump" => _jump, "Hit" => _hit, "Pickup" => _pickup, "Clear" => _clear, _ => null };
		if (source != null) { source.Volume = volume; source.Play(); }
	}
	private void Report(float deltaTime)
	{
		if (_report == null) return;
		_reportTimer += deltaTime;
		if (_reportTimer < 0.1f) return;
		_reportTimer = 0;
		Telemetry.Write(_report, new
		{
			Screen = "Stage", Stage = _stage, X = PlayerPosition.X, Y = PlayerPosition.Y,
			Health = _player!.Health, Energy = _player.Energy, Grounded = _player.Grounded, HighestY = _player.HighestY,
			_player.ShotsFired, _player.Jumps, BossActive, BossHealth = _boss!.Health, Kills = _kills,
			BossX = _boss.Transform.Position.X, BossY = _boss.Transform.Position.Y, Weapon = GameSession.Weapon,
			Elapsed = _elapsed, _player.MoveStrength, _player.Facing, _player.HorizontalSpeed, _player.VerticalSpeed,
			Paused = Entity.Scene.IsPaused, Ending = _endState, GameSession.Cleared, GameSession.Lives, GameSession.Checkpoint,
			Entities = Entity.Scene.EntityCount, Audio = _music!.IsPlaying
		});
	}
	public override void OnDestroy() { _music?.Stop(); }
}
