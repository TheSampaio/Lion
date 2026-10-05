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
