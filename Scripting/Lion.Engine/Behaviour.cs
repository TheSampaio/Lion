namespace Lion.Engine;

/// <summary>Gameplay attached to a native Entity and driven by Lion's component lifecycle.</summary>
/// <remarks>Constructors run before attachment. Use OnAwake for access to Entity and Transform.
/// Callbacks run on the engine thread. A thrown exception stops further updates of this instance.</remarks>
public abstract class Behaviour
{
	/// <summary>The native owner; its reference becomes invalid after native destruction or runtime shutdown.</summary>
	public Entity Entity { get; internal set; }

	/// <summary>Local 2D placement of the owner. Access before OnAwake or after destruction throws.</summary>
	public Transform Transform => Entity.Transform;

	/// <summary>Initializes gameplay when the native owner enters its scene.</summary>
	public virtual void OnAwake() { }

	/// <summary>Receives a direct native owner enable transition.</summary>
	public virtual void OnEnable() { }

	/// <summary>Receives a direct native owner disable transition.</summary>
	public virtual void OnDisable() { }

	/// <summary>Runs in the scene's first update pass, before its main update and physics.</summary>
	/// <param name="deltaTime">The scene timestep in seconds.</param>
	public virtual void OnUpdateBegin(float deltaTime) { }

	/// <summary>Runs once in an active, unpaused scene's main update pass, before physics.</summary>
	/// <param name="deltaTime">The scene timestep in seconds.</param>
	public virtual void OnUpdate(float deltaTime) { }

	/// <summary>Runs in the scene's final update pass, before physics and deferred destruction.</summary>
	/// <param name="deltaTime">The scene timestep in seconds.</param>
	public virtual void OnUpdateEnd(float deltaTime) { }

	/// <summary>Receives the native begin-contact event. Both owners need a native collider.</summary>
	public virtual void OnCollision(Entity other) { }
	/// <summary>Runs in the native render pass for a visible active owner.</summary>
	public virtual void OnRender() { }

	/// <summary>Opts this behaviour into updates while Scene.IsPaused is true.</summary>
	public virtual bool UpdatesWhenPaused => false;

	/// <summary>Releases gameplay resources before the native owner reference is invalidated.</summary>
	/// <remarks>This also runs for a faulted instance. Do not depend on other entities surviving scene teardown.</remarks>
	public virtual void OnDestroy() { }
}
