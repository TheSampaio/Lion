using System.Numerics;
using System.Runtime.InteropServices;
using Lion.Engine;

namespace Lion.Scripting.Tests;

public abstract class AuthoringBase : Behaviour
{
	[Editable] private int _count = 3;
	protected int Count => _count;
}

public sealed class AuthoringProbe : AuthoringBase
{
	[Editable] private float _speed = 12.0f;
	[Editable] public bool Running = true;
	[Editable] public string Message = "Lion café 🦁";
	[Editable] public Vector2 Offset = new(2, 4);
	public override void OnAwake()
	{
		Transform.Position = new Vector2(_speed, Message.Length);
		Transform.Rotation = Count;
		Transform.Scale = Offset;
	}
	public override void OnUpdate(float deltaTime)
	{
		if (Running) { _speed += deltaTime; }
		Transform.Position = new Vector2(_speed, 0);
	}
}

public sealed class LifecycleProbe : Behaviour
{
	public override void OnAwake()
	{
		if (!Entity.IsValid || default(Entity).IsValid || Marshal.SizeOf<TransformState>() != 20)
		{
			throw new InvalidOperationException("Entity or Transform ABI validation failed.");
		}
		Transform.State = new TransformState { Position = new Vector2(1, 2), Rotation = 3, Scale = Vector2.One };
		Task.Run(() =>
		{
			try { _ = Entity.Transform.State; }
			catch (InvalidOperationException) { return; }
			throw new InvalidOperationException("Worker-thread gameplay access was not rejected.");
		}).GetAwaiter().GetResult();
		if (Input.Action("MissingAction").Strength != 0)
		{
			throw new InvalidOperationException("Unknown native action did not return zero.");
		}
	}

	public override void OnUpdateBegin(float deltaTime)
	{
		if (Time.DeltaTime != deltaTime) { throw new InvalidOperationException("Scene timestep was lost."); }
		Transform.Rotation += 1;
	}

	public override void OnUpdate(float deltaTime) => Transform.Position += new Vector2(deltaTime * 100, 0);
	public override void OnUpdateEnd(float deltaTime) => Transform.Scale += Vector2.One;
	public override void OnDisable() => Transform.Rotation = 10;
	public override void OnEnable() => Transform.Rotation = 20;
	public override void OnDestroy() => Transform.Rotation = 99;
}

public sealed class FaultProbe : Behaviour
{
	public override void OnUpdate(float deltaTime)
	{
		Transform.Position += Vector2.One;
		throw new InvalidOperationException("Intentional script failure");
	}
	public override void OnDestroy() => Transform.Rotation = 99;
}

public sealed class LifetimeVictim : Behaviour
{
	internal static Entity FirstOwner;
	internal static bool Captured;
	public override void OnAwake()
	{
		if (!Captured) { FirstOwner = Entity; Captured = true; }
	}
}

public sealed class LifetimeObserver : Behaviour
{
	public override void OnAwake()
	{
		if (LifetimeVictim.FirstOwner.IsValid)
		{
			throw new InvalidOperationException("A destroyed owner's handle remained valid.");
		}
		try { _ = LifetimeVictim.FirstOwner.Transform.State; }
		catch (InvalidOperationException) { Transform.Rotation = 77; return; }
		throw new InvalidOperationException("A stale handle allowed Transform access.");
	}
}

public sealed class ConstructorFault : Behaviour
{
	public ConstructorFault() => throw new InvalidOperationException("Intentional constructor failure");
}

public sealed class AllocationProbe : Behaviour
{
	private int _updates;
	private long _allocated;
	private InputAction _action;
	public override void OnAwake() => _action = Input.Action("MissingAction");
	public override void OnUpdate(float deltaTime)
	{
		var state = Transform.State;
		state.Position.X += deltaTime + _action.Strength;
		Transform.State = state;
		if (Entity.HasComponent<TextRenderer>()) { throw new InvalidOperationException("Unexpected text trait."); }
		_updates++;
		if (_updates == 100) { _allocated = GC.GetAllocatedBytesForCurrentThread(); }
		if (_updates == 1000 && GC.GetAllocatedBytesForCurrentThread() != _allocated)
		{
			throw new InvalidOperationException("Managed hot path allocated after warmup.");
		}
	}
}

public sealed class BindingsProbe : Behaviour
{
	internal static Scene PreviousScene;
	private Entity _temporary;
	private TextRenderer _text = null!;
	private SpriteRenderer _sprite = null!;
	private int _stage;

	public override void OnAwake()
	{
		PreviousScene = Entity.Scene;
		var caption = PreviousScene.FindEntity("Caption") ?? throw new InvalidOperationException("Missing authored entity.");
		if (PreviousScene.FindEntity("Missing") != null) { throw new InvalidOperationException("Missing entity was found."); }
		_text = caption.GetComponent<TextRenderer>() ?? throw new InvalidOperationException("Missing text binding.");
		caption.Name = "Caption café";
		if (caption.Name != "Caption café") { throw new InvalidOperationException("UTF-8 entity name failed."); }
		_text.Text = "C# café ✓";
		if (_text.Text != "C# café ✓") { throw new InvalidOperationException("UTF-8 native text failed."); }
		caption.IsVisible = false;
		caption.IsEnabled = false;
		if (caption.IsVisible || caption.IsActive) { throw new InvalidOperationException("Native entity state failed."); }
		caption.IsVisible = true;
		caption.IsEnabled = true;
		_sprite = caption.GetComponent<SpriteRenderer>() ?? throw new InvalidOperationException("Missing sprite binding.");
		_sprite.Order = 17;
		_sprite.FlipX = true;
		_sprite.FlipY = true;
		if (_sprite.Order != 17 || !_sprite.FlipX || !_sprite.FlipY) { throw new InvalidOperationException("Sprite state failed."); }
		try { _ = TextureAsset.FromPath("../Outside.png"); throw new Exception("Traversal accepted."); }
		catch (ArgumentException) { }
		try { PreviousScene.Load("Scenes/Main.lnscene"); throw new Exception("Awake scene transition accepted."); }
		catch (InvalidOperationException) { }
		_temporary = PreviousScene.CreateEntity("Temporary");
		_temporary.Transform.Position = new Vector2(10, 20);
		_temporary.Destroy();
		_temporary.Destroy();
	}

	public override void OnUpdate(float deltaTime)
	{
		if (_stage == 0 && !_temporary.IsValid) { throw new InvalidOperationException("Destroy was not deferred."); }
		if (_stage == 1 && _temporary.IsValid) { throw new InvalidOperationException("Destroyed entity token survived."); }
		if (_stage == 2)
		{
			if (_sprite.IsValid) { throw new InvalidOperationException("Removed component view remained valid."); }
			try { _ = _sprite.Order; throw new Exception("Removed trait accepted access."); }
			catch (InvalidOperationException) { }
		}
		_text.IsEnabled = !_text.IsEnabled;
		Transform.Position = new Vector2(++_stage, 0);
	}
}

public sealed class BindingLifetimeObserver : Behaviour
{
	public override void OnAwake()
	{
		if (BindingsProbe.PreviousScene.IsValid) { throw new InvalidOperationException("Cleared scene token survived."); }
		try { BindingsProbe.PreviousScene.FindEntity("Caption"); }
		catch (InvalidOperationException) { Transform.Position = new Vector2(78, 0); return; }
		throw new InvalidOperationException("Cleared scene accepted a lookup.");
	}
}

public sealed class GameplayCompanion : Behaviour
{
	internal bool Awoke;
	public override void OnAwake() => Awoke = true;
}

public sealed class GameplayProbe : Behaviour
{
	private Entity _ground;
	private BoxCollider2D _groundBox = null!;
	private RigidBody2D _body = null!;
	private int _updates, _contacts;
	private long _allocated;
	public override bool UpdatesWhenPaused => true;
	public override void OnAwake()
	{
		const string resourceText = "Resource café ✓\n";
		if (Resources.ReadText("Plain.txt") != resourceText || Resources.ReadText("Sealed.txt") != resourceText
			|| Resources.ReadBytes("Empty.txt").Length != 0) throw new Exception("Plain/sealed resource reading failed.");
		bool windowRejected = false;
		try { _ = Window.Size; } catch (InvalidOperationException) { windowRejected = true; }
		if (!windowRejected || Application.IsEditor) throw new Exception("Headless host contract failed.");
		var scene = Entity.Scene;
		scene.Gravity = new Vector2(0, -1);
		if (scene.Gravity != new Vector2(0, -1)) throw new Exception("Gravity round trip failed.");
		_ground = scene.CreateEntity("Managed Ground"); _ground.Transform.Position = new Vector2(0, -50);
		_ground.AddComponent<RigidBody2D>(body => body.Configure(BodyType.Static));
		_groundBox = _ground.AddComponent<BoxCollider2D>(box => box.Configure(new Vector2(80, 20), friction: 0));
		_body = Entity.AddComponent<RigidBody2D>(body => body.Configure(BodyType.Dynamic, true));
		Entity.AddComponent<BoxCollider2D>(box => box.Configure(new Vector2(10), friction: 0));
		_body.LinearVelocity = new Vector2(0, -120);
		if (_body.BodyType != BodyType.Dynamic || !_body.IsFixedRotation) throw new Exception("Rigid body configuration failed.");
		if (!scene.Raycast(new Vector2(0, 30), new Vector2(0, -200), out var hit, Entity) || hit.Entity != _ground || hit.Normal.Y < 0.9f) throw new Exception("Native raycast failed.");
		var child = scene.CreateEntity("Hierarchy Child"); child.Transform.Position = new Vector2(7, 9); child.SetParent(Entity, false);
		if (Entity.ChildCount != 1 || Entity.GetChild(0) != child || child.Parent != Entity || child.WorldTransform.Position != new Vector2(7, 9)) throw new Exception("Hierarchy round trip failed.");
		var combo = child.AddComponent<ComboBox>(value => value.SetField("Options", "First|Second"));
		combo.SetPrefix("Mode: "); combo.SelectedIndex = 1;
		if (!child.GetComponent<TextRenderer>()!.Text.Contains("Second")) throw new Exception("Dropdown label was not refreshed.");
		child.SetEnabled(false);
		bool initializerRejected = false;
		try { child.AddComponent<AudioPlayer>(_ => throw new ArgumentException("Intentional initializer failure")); }
		catch (ArgumentException) { initializerRejected = true; }
		if (!initializerRejected) throw new Exception("Initializer exception was swallowed.");
		try { Entity.SetParent(child); throw new Exception("Hierarchy cycle accepted."); } catch (InvalidOperationException) { }
		var camera = Entity.AddComponent<Camera2D>(); camera.SetField("Offset", new Vector2(4, 5));
		if (camera.GetField<Vector2>("Offset") != new Vector2(4, 5)) throw new Exception("Vector2 reflection failed.");
		var text = Entity.AddComponent<TextRenderer>(); text.Text = "Reflection café"; text.SetField("Color", new Vector3(0.2f, 0.4f, 0.6f));
		if (text.GetField<string>("Text") != text.Text || text.GetField<Vector3>("Color").Y != 0.4f) throw new Exception("Native reflection failed.");
		text.SetField("Size", 23f); text.SetField("Order", 12); text.SetField("Centered", false);
		if (text.GetField<float>("Size") != 23 || text.GetField<int>("Order") != 12 || text.GetField<bool>("Centered")) throw new Exception("Reflected scalar type mismatch.");
		var button = Entity.AddComponent<Button>(); button.IsSelected = true;
		if (!button.IsSelected) throw new Exception("Button state failed.");
		button.IsEnabled = false;
		var check = Entity.AddComponent<CheckBox>(); check.IsChecked = true;
		if (!check.IsChecked) throw new Exception("Checkbox state failed.");
		check.IsEnabled = false;
		var bar = Entity.AddComponent<ProgressBar>(); bar.SetRange(2, 8); bar.Value = 40;
		if (bar.Value != 8) throw new Exception("Progress range failed.");
		bar.IsEnabled = false;
		var audio = Entity.AddComponent<AudioPlayer>(source => source.PlayOnAwake = false); audio.Volume = 0.4f; audio.Pitch = 1.2f;
		if (audio.IsPlaying || audio.Volume != 0.4f || audio.Pitch != 1.2f) throw new Exception("Audio state failed.");
		Entity.AddComponent<ParticleEmitter>().EmitAt(Vector2.Zero, 4);
		var companion = Entity.AddBehaviour<GameplayCompanion>();
		if (!companion.Awoke || !ReferenceEquals(companion, Entity.GetBehaviour<GameplayCompanion>())) throw new Exception("Dynamic managed behaviour failed.");
		Entity.RemoveBehaviour<GameplayCompanion>();
		Transform.Scale = new Vector2(1, 91);
	}
	public override void OnUpdate(float deltaTime)
	{
		_updates++;
		if (_updates == 2 && Entity.GetBehaviour<GameplayCompanion>() != null) throw new Exception("Managed removal was not deferred safely.");
		if (_updates == 2 && Entity.GetChild(0).HasComponent<AudioPlayer>()) throw new Exception("Failed initializer did not roll back at the safe boundary.");
		if (_updates == 20) _ground.RemoveComponent<BoxCollider2D>();
		if (_updates == 21 && (_groundBox.IsValid || Entity.Scene.RaycastNormal(new Vector2(0, 50), new Vector2(0, -200), out _, Entity))) throw new Exception("Removed native shape remained queryable.");
		// Warm and then verify the native velocity/query path without managed view or text allocation.
		_ = _body.LinearVelocity;
		_ = Entity.Scene.RaycastNormal(new Vector2(1000, 50), new Vector2(0, -200), out _, Entity);
		if (_updates == 100) _allocated = GC.GetAllocatedBytesForCurrentThread();
		if (_updates == 1000 && GC.GetAllocatedBytesForCurrentThread() != _allocated) throw new Exception("Physics hot path allocated.");
		Transform.Scale = new Vector2(_updates, _contacts > 0 ? 92 : 91);
	}
	public override void OnCollision(Entity other)
	{
		if (other == _ground) _contacts++;
	}
}

public sealed class RenderProbe : Behaviour
{
	public override void OnRender() => Transform.Position += Vector2.UnitX;
}
