# C# scripting architecture

## Status and implementation sequence

This is an incremental SDK, not a migration of Brickout. Existing native games keep their module,
entry point, components, scene files, and build workflow. The first increment is an opt-in Windows
x64 host plus an executable native/managed integration test. The second increment adds Mane
scaffolding, project compilation, named component attachment, editable fields and Play/Stop ownership.
C# player export remains unsupported and is rejected before creating an incomplete player.

1. **Foundation:** .NET hosting, assembly discovery, component lifecycle adapter, safe entity
   references, local Transform access, input actions, Time, logging, exception isolation, and tests.
2. **Authoring:** stable script registration names, cached field metadata, Reflector/Serializer
   adapters, ComponentScripts C# scaffolding, generated project builds, and Play/Stop ownership.
3. **Player:** managed build output and app-local .NET runtime packaging in ProjectExporter and
   the Windows export template. Verify clean-machine startup before advertising C# export support.
4. **Gameplay coverage:** scene operations and typed native component wrappers, followed by
   physics, audio, particles, rendering, UI, and assets. Only expose implemented native operations.
5. **Reload and profiling:** collectible game contexts, state restoration, debugger workflows,
   and benchmarks for the additional bindings.

## Repository audit

| Area | Current owner and behavior | Scripting decision |
| --- | --- | --- |
| Application | `Core/Application.cpp`: layers update, render, swap; input/audio are serviced by the native application | Keep the native loop; initialize .NET only for projects that use it |
| Entity | `Logic/Entity`: final composed object, local Transform, hierarchy, ordered owned components, process-local picking ID | Managed Entity is a value referencing a validated lifetime token, never a pointer or a subclass |
| Components | `Component`/`ComponentRegistry`: OnAttach, Awake, three update passes, enable/disable, collision, destroy; registry factories belong to modules | A native adapter calls managed instances; preserve the existing lifecycle and registration ABI |
| Scene | `Scene`: entity list, deferred subtree removal, three update passes, physics, then removals | Do not duplicate the container; invalidate scripting handles when native destruction finishes |
| Transitions | `SceneManager`: deferred load while updating, serializer loading, Clear on replacement | Future scene bindings must use the same deferred path, including Play/Stop and reload |
| Transform/math | 2D pixel position/scale, scalar rotation in degrees, parent-composed world accessors | Use `System.Numerics.Vector2`; exchange a plain five-float local Transform snapshot, not GLM memory |
| Input | `Core/Input`: keyboard/mouse/gamepads, resource-relative sealed action map, strength/press/tap | Intern named actions once; reuse the native action evaluation and remapping |
| Physics | Scene-owned Box2D world, 100 pixels/meter, 60 Hz accumulator, capped substeps, contact callbacks | Bind existing RigidBody2D/collider capabilities later; no public fixed-update hook until the native loop exposes one |
| Rendering | Sprite batches, orthographic/Camera2D framing, framebuffer picking, native post-processing | Keep rendering native; later bind SpriteRenderer, TextRenderer, Camera2D and PostProcessingComponent |
| Particles | `ParticleComponent`: reserved CPU storage and cached shared textures | Expose the existing emitter, not a second managed particle simulation |
| Audio | XAudio2, native voice IDs, buses, AudioClip cache, AudioPlayer scene component | Bind the mixer and existing component; do not expose voice pointers |
| Assets | Named shared texture/audio/font caches; project override and executable fallback; Vault unsealing | Preserve resource-relative identity; introduce typed references when the actual binding needs them |
| Serialization | SceneSerializer owns JSON; Component sees abstract Serializer/Reflector; Assemblies are linked rooted definitions | Managed metadata must feed those boundaries; never pass JSON or implementation languages through scenes |
| Editor | Mane serializes/reconstructs on Play/Stop and native module reload; Inspector consumes reflection | Scripts must remain dormant while authoring; runtime activation belongs to the Play boundary |
| Source/build | ComponentScripts owns language scaffolding; ProjectBuild compiles generated native modules against the packaged SDK | Add the managed backend here; do not branch the entity model or modify authored C++ scripts |
| Events/UI | Layer window/input events; Component contact callbacks; native Button/CheckBox/ComboBox/ProgressBar | Preserve ownership and use lifecycle-scoped subscriptions when event bindings are introduced |
| Packaging | Native launcher loads fixed `lion-game.dll`; exporter builds Shipping and seals resources | Keep launcher free of game behavior; future C#-only projects still need a supplied native bootstrap |

Two lifecycle constraints matter for later tooling: Scene::Add calls Awake even during authoring,
and component enable callbacks currently follow direct owner toggles rather than inherited hierarchy
transitions. Do not silently introduce different C# semantics. Dynamic component mutation during a
native component-vector iteration also needs a deferred contract before exposing Add/Remove from C#.

## Runtime and interop decision

Use the official .NET hosting ABI (`hostfxr_initialize_for_runtime_config`,
`hostfxr_get_runtime_delegate`, `load_assembly_and_get_function_pointer`) to load `Lion.Engine.dll`.
The first increment takes an explicit absolute hostfxr path; installation discovery and app-local
distribution are subsequent tooling work. It has no link-time dependency on .NET, so C++ games run
without a managed runtime. Native hosting declarations are private, limited to the documented Windows
x64 ABI, and must be checked against the SDK headers when extending the host.

The permanent SDK bootstrap returns unmanaged function pointers and accepts a size/version-tagged
native function table. Both sides validate the entire v2 table before binding. Calls use fixed-width
values, UTF-8 at setup/error boundaries, status codes, and plain sequential structs. No STL, GLM,
JSON, object references, exceptions, or allocator ownership cross this boundary.

| Alternative | Tradeoff |
| --- | --- |
| hostfxr + function tables (chosen) | In-process modern .NET, explicit ABI compatibility, one binding step, no symbol lookup per gameplay call; requires table/lifetime tests |
| LibraryImport | Appropriate for independent native services, but many per-operation exports couple the SDK to the DLL symbol surface |
| Delegate marshalling | Easier prototype but adds roots, callback ownership and extra indirection; avoid temporary delegates in updates |
| NativeAOT gameplay | Attractive startup/deployment but does not provide the desired dynamic script discovery and future assembly reload workflow |
| Mono/legacy .NET Framework | Adds a separate runtime/dependency ecosystem without a demonstrated requirement in this Windows C++20 engine |

The installed .NET 10 SDK builds the initial `net10.0` API. It is an optional scripting prerequisite,
not a prerequisite for `Scripts/Build.bat`. All managed build artifacts go under `Build/Managed`.

References: [Microsoft native hosting guide](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting),
[runtime hosting ABI](https://github.com/dotnet/runtime/blob/main/docs/design/features/native-hosting.md),
[assembly unloadability](https://learn.microsoft.com/en-us/dotnet/standard/assembly/unloadability).

## Projects and public surface

- `Scripting/Lion.Engine`: documented public SDK in `Lion.Engine`; bootstrap and ABI under
  `Lion.Engine.Internal`, effectively internal even where host reflection requires a public method.
- `Engine/Source/Lion/Scripting`: native host and Component lifecycle adapter inside the shared core.
- `Scripting/Examples`: actual scripts compiled against the implemented SDK.
- `Scripting/Tests`: native executable drives real scenes through the hosted runtime; no fake engine.

The first public surface is Behaviour, Entity, Transform/TransformState, Input/InputAction, Time,
and Log. Public math uses System.Numerics. Future component wrappers must use Lion's existing names
(RigidBody2D, Camera2D, AudioPlayer, etc.), not invented Unity component types.

## Ownership, lifecycle, and threading

The native Scene/Entity remain the owners. CSharpScript is an opt-in native Component adapter and
is registered once per discovered stable full script name, using the same language-neutral registry
as native components. Named registration does not overwrite the C++ type-to-name mapping: different
managed scripts share one native adapter class but keep distinct authored identities.

Mane initializes the host/catalog before loading its scene and keeps gameplay inactive while editing.
The adapter owns a native field cache, not an authoring Behaviour instance. Metadata discovery caches
compiled field accessors; a temporary unattached default instance is constructed once when the field
schema is first requested. Constructors must only initialize data, never run gameplay or subscribe
to events. Play rebuilds the edited snapshot with gameplay active, applies its fields before Awake,
then uses the native update passes. Stop clears live scripts before restoring the inactive snapshot.

Managed instances live in a bootstrap-owned dictionary, keyed by monotonic instance tokens.
Managed Entity contains only a monotonic native lifetime token. The native table stores weak entity
references; destruction invalidates the token even if an editor or C++ caller retains the Entity.
Tokens are never reused across runtime shutdown/reinitialization. Saved scenes never contain these
runtime tokens. Destroy callbacks run before owner invalidation so cleanup can still access Transform.

UpdateBegin -> Update -> UpdateEnd occurs before native physics, followed by deferred removals.
Disabled entities/ancestors and disabled components suppress updates through the existing native
dispatch. Hidden entities still update. Pause follows native Component behavior. No OnFixedUpdate,
OnStart, trigger-end event, or physics query is invented in this increment. Scene delta time is
passed explicitly to managed callbacks; Time.DeltaTime is callback-scoped.

All runtime and gameplay operations are main-thread-only. Native callbacks reject another thread
before accessing maps or entities. Managed bootstrap entry points check their owning thread before
touching their caches. Worker-thread support is not promised. No managed exception may cross an
unmanaged entry point. A fault disables that instance's subsequent gameplay callbacks but still
permits Destroy; diagnostics include callback, script type, exception/stack, and native owner context.

The .NET process runtime and permanent SDK bootstrap are not unloaded on Shutdown. Each game assembly
uses a collectible context and an AssemblyDependencyResolver, sharing the bootstrap's Lion.Engine
assembly explicitly. Shutdown releases instances, discovery metadata and native tokens, and requests
context unload. Mane compiles to the original project output but loads a unique private build copy,
so a loaded assembly does not prevent recompilation. A successful Compile clears the old scene,
instances, registry entries and metadata before loading the new catalog and reconstructing the scene.
Compiler failures keep the live build; invalid new metadata restores the previous private catalog
for the same project when available. This is an explicit scene-reconstructing reload, not in-place
hot reload. Verified collection and state-preserving gameplay hot reload are future work: an
AssemblyLoadContext is not a sandbox, and its Unload call alone does not prove collection.

Discovery caches constructors and overridden-callback masks. Native adapters skip unimplemented
callbacks before crossing into managed code. Entity attachment creates one cached Transform wrapper;
property access and update dispatch allocate no wrappers. Input names are interned once per session.

## Serialization and editor integration plan

Use the full script type name in the registry; never persist a native address, DLL type index, language
tag, or machine path. `[Editable]` opts mutable instance fields into cached metadata. Float, int, bool,
string and Vector2 map to the existing abstract Reflector/Serializer. Vector2 uses `.x`/`.y` archive
keys and the shared two-axis Inspector control. Private/inherited fields are supported; readonly,
static, hidden duplicate names and unsupported types fail discovery with a field-specific diagnostic.
Properties, enums, references, lists and additional attributes are not implemented. Renaming fields
changes their saved keys; incompatible saved types report an error and restore the script's defaults.
Null strings round-trip as empty strings. Reflection is acceptable at load, not in update loops.

`ComponentScripts` now offers C# alongside C++. `ProjectScripting` generates a managed build under
the project's Build directory, referencing the Managed SDK beside Mane rather than engine sources.
Compile builds the generated native bootstrap through the project solution and builds all C# assets
into `lion-scripts.dll`; Visual Studio C++ tools and the .NET 10 SDK are still required. Game developers
do not need to edit that bootstrap. The same flow is available through `--compile-project` for CI.
`PackManagedSdk.bat` packages the API when the .NET 10 SDK exists; native engine builds still work
without it. Mane locates installed Windows x64 hostfxr in DOTNET_ROOT_X64, DOTNET_ROOT or ProgramFiles.

Player export must still include compiled assemblies, matching runtime configuration, SDK API and
app-local runtime, with licenses and clean-machine tests. The exporter currently rejects projects
containing C# source instead of silently exporting only their native bootstrap.

## Extending the SDK

For a new binding, identify the existing native owner first. Add a narrowly scoped status-returning
function to the private table, increment its ABI version/size, implement validated lookup and native
exception containment, then add one idiomatic documented public operation. Verify both ABI layouts,
stale handles, thread misuse and native behavior. Resolve names/assets at load rather than per frame.

For a new native component wrapper, use its stable registry name, validate both entity lifetime and
component attachment lifetime, and forward operations to the existing component. Do not retain a
raw Component pointer. Adding/removing a component must obey the native iteration/deferred-mutation
contract. For a managed Behaviour, derive from Behaviour and implement only the callbacks needed.

Performance measurements must separate cold assembly/discovery cost, JIT warmup, native callback
dispatch, and managed Transform operations. The first test runner measures a real native scene with
many managed updates and checks managed per-thread allocation after warmup. Later benchmarks should
compare individual native/managed operations under the same configuration, without frame logging.

## Foundation verification

The first increment was verified with native Debug, Release and Shipping builds, plus the hosted
integration runner in each configuration. Managed projects compile with zero warnings. The runner
checks real scene callbacks and Transform writes, lifetime invalidation/restart, worker-thread
rejection, exception isolation and cleanup, then compares 256 entities over 1000 scene updates.

One local Release run measured 11.57 ms for native components and 68.66 ms for hosted C# components;
Shipping measured 11.18 ms and 75.65 ms respectively. The managed benchmark reported no per-thread
managed allocations after its initial 100-frame warmup. These are single-process foundation checks,
not statistically controlled benchmarks, a claim of equal native performance, or proof that arbitrary
user scripts allocate nothing. Native/managed transitions remain measurable overhead.

Mane and the native Brickout launcher were also run in all three configurations. The second increment
extends verification with native authoring/Play/Stop round trips, editable inherited/private fields,
distinct adapter identities, collision rejection, unsupported metadata, reentrant Inspector walks,
incompatible archive recovery, generated-project builds and the managed-export guard. The UI fixture
is under `Scripting/Tests/EditorProject`; run `Scripts/VerifyCSharpEditor.ps1` to create a Build-local copy.
App-local runtime export and proven assembly collection still belong to subsequent increments.
