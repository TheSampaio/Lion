# Build a complete C# game

Open `Scripting/Examples/StarCollector` in Mane, Compile, then Play. The project contains only C#
gameplay and scene-authored native traits: menu, movement, pickups, countdown, HUD, victory/defeat,
restart and quit. Enter/Space starts or retries, arrows/WASD move, and Escape quits or stops Play.
The existing Brickout Sandbox remains native and is not migrated.

For a larger gameplay proof, open [Aster Circuit](../../Scripting/Examples/AsterCircuit/README.md).
It is an original action platformer with three themed stages, bosses, weapon unlocks, checkpoints,
local progress, native physics, UI, particles and original synthesized music. All client gameplay
is C#. The [coverage reference](csharp-parity.md) distinguishes supported gameplay from core internals.

Automatic managed startup belongs to the supplied scene player. A developer-owned native entry
point retains ownership of hosting, scene cleanup and shutdown; it must initialize CSharpRuntime
explicitly if it also uses C# gameplay. Packaging managed files alone does not change a custom loop.

## Authoring to distribution

1. Create a project and author entities with Camera 2D, Sprite Renderer and Text Renderer in Mane.
2. Create a C# Behaviour through Add Component / New Component. Compile and attach its full type
   name. Use `[Editable]` fields for float, int, bool, string and Vector2 configuration.
3. Define actions in the native input map. Resolve entities, traits and InputAction values in OnAwake;
   update their state in the three native update callbacks. Do not access Entity in constructors.
4. Save the entry scene and project. Compile builds scripts against the packaged `Lion.Engine.dll`.
   C#-only projects use a precompiled scene-player module; no C++ source/compiler is needed.
5. Play uses the authored scene; Stop restores its pre-Play snapshot. Compile reconstructs the scene
   with a newly loaded script catalog. This is not state-preserving gameplay hot reload.
6. Export Windows. Mane builds Shipping scripts, seals packaged assets and includes the SDK API,
   scripts, app-local .NET 10 runtime and required licenses. Run the exported executable from any
   working directory. Neither the .NET SDK nor a system .NET installation is needed by the player;
   existing Windows x64 / Visual C++ native runtime prerequisites still apply.

For automation, use `Lion.exe --compile-project <project>` and
`Lion.exe --export-windows <project> <destination>`, waiting for exit and checking the exit code.
All generated files and local export verification artifacts belong under `Build/`, which Git ignores.

## Implemented runtime contracts

| Operation | Ownership and timing |
| --- | --- |
| `Entity.Scene`, `Scene.FindEntity(name)` | Native scene, exact case-sensitive first match. Cache references outside update loops. All authored entities are attached before any Awake runs. |
| `Scene.CreateEntity(name)` | Native-owned bare entity; compose it with AddComponent/AddBehaviour. |
| `Entity.Destroy()` | Idempotent deferred subtree removal. The reference stays valid until the native scene completes removal. |
| `Entity.Name`, `IsEnabled`, `IsVisible`, `IsActive` | Native state; hidden entities still update. IsActive follows enabled ancestry. |
| `HasComponent<T>()`, `GetComponent<T>()` | Fourteen native views; existence checks allocate no wrappers. Cache GetComponent views. Managed scripts use GetBehaviour<T>. |
| `AddComponent<T>(initializer)`, `RemoveComponent<T>()` | Configure before Awake; removal completes after physics. Adding the same native trait twice is rejected. |
| `AddBehaviour<T>()`, `GetBehaviour<T>()`, `RemoveBehaviour<T>()` | Registered managed scripts remain native component adapters; dynamic addition runs Awake immediately. |
| `Parent`, `GetChild`, `SetParent`, `WorldTransform` | Native hierarchy, validated same-scene parenting and cycle rejection. |
| `Scene.Instantiate(path)` | Native Assembly reconstruction; complete hierarchy attaches before Awake. |
| `RigidBody2D`, colliders, `Scene.Raycast` | Native simulation and pixel-space queries; velocity/ground-normal paths allocate no managed views. |
| `AudioPlayer`, `Scene.Audio` | Native source playback, voice IDs, mixer buses. Stop explicit mixer voices; scene-owned sources clean up automatically. |
| `Button`, `CheckBox`, `ComboBox`, `ProgressBar`, `WidgetAnchor` | Native widget state and reflected style/configuration. |
| `Camera2D`, `PostProcessing`, `ParticleEmitter` | Native framing/effects/emission, not managed replacements. |
| `SpriteRenderer.Texture`, `Order`, `FlipX`, `FlipY` | Forward to the current native trait. TextureAsset is a resource-relative identity; changing textures can perform asset loading. |
| `TextRenderer.Text`, component `IsEnabled` | Native bitmap text/trait state. String reads and changes allocate/encode; avoid rewriting an unchanged HUD every frame. |
| `Scene.Load(path)` | Active-scene update/contact callbacks only. Resource-relative forward-slash path; deferred through SceneManager. |
| `Application.RequestQuit()` | Closes the standalone game or requests Mane to Stop Play. |
| `Resources.ReadText/ReadBytes` | Authored override/executable fallback, native Vault unsealing, maximum 64 MiB decoded. Loading path, not per-frame. |
| `Window` | Standalone title, dimensions, icon, clear color and state. Mutations are rejected in Mane Play. |

All engine access is main-thread-only. Default/stale Entity or Scene values are invalid; guarded
operations throw InvalidOperationException. Clear and scene transitions invalidate old scene and
entity tokens, even when a native caller retains the objects. Component views store no raw pointer:
removal invalidates a view, and replacing the same trait on its living owner retargets that view.
Saved data never contains runtime tokens or machine-specific asset paths.

The SDK caches discovery metadata and overridden callback masks. Transform access, state queries,
cached input and native trait existence checks allocate no managed objects after warmup. Name lookup,
entity creation, text/texture changes and user code are not promised allocation-free.

## Configuration and hot paths

Each native view exposes `GetField<T>(name)` / `SetField<T>(name, value)` using the native Reflect()
schema, with float, int, bool, string, Vector2 and Vector3 values. `SetAssetField` validates portable
resource-relative identities. Use these for setup, not per-frame animation: they encode field names
and box values. Prefer the strongly typed live operations for audio, sprites, body motion and widgets.
Body.Configure sets initial settings, before Awake; collider.Configure plus RefreshShape applies
live shape changes. Generic reflection is not a substitute for a setter's asset-cache side effects.

Native actions are zero-to-one strengths. Compose a signed axis as Right.Strength - Left.Strength;
use opposite-signed gamepad-axis bindings on the two actions. InputAction.WasPressed fires on a
press edge. The older raw native GetKeyTap fires on release after a press; it is not the same event.
Behaviour.OnCollision receives native contact-begin ownership, OnRender participates in the native
render pass, and UpdatesWhenPaused opts a controller into paused updates.

Richer editable references/collections, trigger-end callbacks, state-preserving script reload and
runtime input-map rebinding are not exposed yet. Core-only renderer buffers, OS/editor integration
and native module ownership are intentionally not public gameplay APIs. See [coverage](csharp-parity.md).

`Scripts/VerifyCSharp.ps1` tests native ownership, lifecycle, serialization, thread misuse, exceptions,
UTF-8 bindings and allocation. `Scripts/VerifyCSharpEditor.ps1` builds/exports a fresh fixture and runs
its C# update with invalid system DOTNET_ROOT settings and a minimal PATH, checking that framework
and scripts actually load from the package. This is runtime-isolation testing, not clean-VM Windows
prerequisite certification. The host uses the official
[explicit dotnet_root initialization contract](https://github.com/dotnet/runtime/blob/main/docs/design/features/native-hosting.md).

See the [0.43.0 verification record](csharp-verification.md) for executed checks and remaining
certification boundaries.
