# Build a complete C# game

Open `Scripting/Examples/StarCollector` in Mane, Compile, then Play. The project contains only C#
gameplay and scene-authored native traits: menu, movement, pickups, countdown, HUD, victory/defeat,
restart and quit. Enter/Space starts or retries, arrows/WASD move, and Escape quits or stops Play.
The existing Brickout Sandbox remains native and is not migrated.

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
| `Scene.CreateEntity(name)` | Native-owned bare entity; returns a lifetime-validated Entity. Does not attach arbitrary components. |
| `Entity.Destroy()` | Idempotent deferred subtree removal. The reference stays valid until the native scene completes removal. |
| `Entity.Name`, `IsEnabled`, `IsVisible`, `IsActive` | Native state; hidden entities still update. IsActive follows enabled ancestry. |
| `HasComponent<T>()`, `GetComponent<T>()` | Supported native views are SpriteRenderer and TextRenderer. Trait queries allocate no managed objects; cache views returned by GetComponent. Not a managed Behaviour lookup API. |
| `SpriteRenderer.Texture`, `Order`, `FlipX`, `FlipY` | Forward to the current native trait. TextureAsset is a resource-relative identity; changing textures can perform asset loading. |
| `TextRenderer.Text`, component `IsEnabled` | Native bitmap text/trait state. String reads and changes allocate/encode; avoid rewriting an unchanged HUD every frame. |
| `Scene.Load(path)` | Active-scene update callbacks only. Resource-relative forward-slash path. Queued through SceneManager, never destroys the executing scene mid-callback. |
| `Application.RequestQuit()` | Closes the standalone game or requests Mane to Stop Play. |

All engine access is main-thread-only. Default/stale Entity or Scene values are invalid; guarded
operations throw InvalidOperationException. Clear and scene transitions invalidate old scene and
entity tokens, even when a native caller retains the objects. Component views store no raw pointer:
removal invalidates a view, and replacing the same trait on its living owner retargets that view.
Saved data never contains runtime tokens or machine-specific asset paths.

The SDK caches discovery metadata and overridden callback masks. Transform access, state queries,
cached input and native trait existence checks allocate no managed objects after warmup. Name lookup,
entity creation, text/texture changes and user code are not promised allocation-free.

## Scope and next increments

This supports a complete small scene-authored game, not every native engine capability from C#.
Physics/contact/query APIs, audio playback, UI events, particles, hierarchy editing, runtime Assembly
instantiation, arbitrary component attachment and richer editable references/collections are not yet
bound. Native versions can still be authored in scenes where their existing behavior is sufficient.
Do not use an invented API or reflection into interop internals to work around those boundaries.

`Scripts/VerifyCSharp.ps1` tests native ownership, lifecycle, serialization, thread misuse, exceptions,
UTF-8 bindings and allocation. `Scripts/VerifyCSharpEditor.ps1` builds/exports a fresh fixture and runs
its C# update with invalid system DOTNET_ROOT settings and a minimal PATH, checking that framework
and scripts actually load from the package. This is runtime-isolation testing, not clean-VM Windows
prerequisite certification. The host uses the official
[explicit dotnet_root initialization contract](https://github.com/dotnet/runtime/blob/main/docs/design/features/native-hosting.md).

See the [0.43.0 verification record](csharp-verification.md) for executed checks and remaining
certification boundaries.
