# Native gameplay coverage — 0.44.0

The C# SDK forwards gameplay to Lion's existing native owners. It is not a second engine and does
not expose raw Box2D IDs, GPU buffers or DLL pointers. This is gameplay coverage, not literal parity
with every public C++ header. The following surface is implemented; do not invent missing methods.

| Native owner | C# gameplay surface |
| --- | --- |
| Scene / SceneManager | Find/Create/GetEntity, entity count, native trait queries, deferred Load, pause, gravity, Assembly Instantiate |
| Entity / Transform | Lifetime validation, name/enabled/visible/active, hierarchy, world/local transform, scene order, deferred Destroy |
| Component registry | Add/Get/Has/Remove native traits; Add/Get/Remove registered Behaviours; configure-before-Awake |
| RigidBody2D | Body type/fixed-rotation initial configuration, velocity, teleport; no force/impulse invented where the native component has none |
| BoxCollider2D / CircleCollider2D | Dimensions, density/friction/restitution, configure/refresh, shape destruction on removal |
| PhysicsWorld | Closest-hit pixel-space raycast, optional ignored owner; allocation-free normal-only query; native contact-begin callback |
| SpriteRenderer / Sprite | Resource texture, draw order, flips, normalized UV region, pixel display size, RGB modulation |
| TextRenderer | UTF-8 text, copy style, native bitmap-font/configuration fields |
| Camera2D | Zoom, resolved view, offset, limits and smoothing through shared reflection |
| AudioPlayer / Audio | Source path/loop/volume/pitch/bus/play/stop/state; scene mixer voices and bus volume |
| ParticleComponent | ParticleEmitter burst/position; shared reflected texture, lifetime, motion, size and color configuration |
| PostProcessingComponent | PostProcessing fade plus native reflected finishing-effect fields |
| Button / CheckBox / ComboBox / ProgressBar / WidgetAnchor | Native click/hover/selection/value states, operations, style/configuration and screen anchoring |
| Input | Cached named action strengths/press edges, pointer, raw key/mouse/gamepad values, last input family |
| Asset / Filesystem / Vault | TextureAsset identity, source/font/particle asset fields, plain/sealed resource reads, project override and executable fallback |
| Window / Application / Clock / Log | Standalone host settings, editor identity, quit, callback timestep, native verbosity-aware logging |

All fourteen native component views use the same Reflect schema as the Inspector. SetField/GetField
support float, int, bool, string, Vector2 and Vector3; asset fields reject absolute/traversal paths.
Field access allocates and is intended for setup. Runtime setters with side effects (texture/clip
replacement, UI state, motion and UVs) have explicit methods/properties. Sprite region/color remain
runtime sprite state, like their native C++ operations, rather than additional serialized scene fields.

## Composition example

```csharp
var actor = Entity.Scene.CreateEntity("Courier");
actor.Transform.Position = new Vector2(192, 155);
actor.AddComponent<RigidBody2D>(body => body.Configure(BodyType.Dynamic, true));
actor.AddComponent<BoxCollider2D>(box => box.Configure(new Vector2(54, 78), friction: 0));
actor.AddBehaviour<Courier>();
```

Cache component views and action values in Awake. Use Transform.State for grouped transform edits,
LinearVelocity for physical movement and RaycastNormal for a per-frame grounded check. SetPosition
teleports the physics body; changing Transform alone does not replace simulation ownership.

## Explicit boundaries

- Editor fields on managed scripts still support float/int/bool/string/Vector2, not arbitrary
  collections, enums or object references. Runtime C# may use normal .NET data structures.
- Named action maps are authored in Mane/assets. Runtime rebinding/map serialization is not yet a
  C# binding. Raw key tap follows native release semantics; action WasPressed is a press edge.
- Contact-begin is available. Native triggers, contact-end events, force APIs and overlap queries
  must first exist as supported native component/world capabilities before being promised here.
- Explicit mixer voices are process-level and must be stopped. Prefer scene-owned AudioPlayer for
  music that must end on a scene transition. File persistence uses .NET in the user's data directory.
- Core rendering backend objects, native entry-point/layer ownership, editor documents and OS shell
  services are engine/tool responsibilities, not unsafe handles exposed to client scripts.
- Reload reconstructs the scene; state-preserving gameplay reload and proven collectible-context
  reclamation remain separate work. All Lion operations require the engine thread.

Aster Circuit is the executable proof of the covered path. A commercial release still needs human
playtesting, accessibility/controller QA, art/audio refinement and clean-machine distribution checks.
