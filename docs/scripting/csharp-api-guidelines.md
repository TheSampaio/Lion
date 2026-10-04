# Lion C# API guidelines

The public SDK serves game developers, not the shape of the C++ implementation. Keep its supported
surface small, consistent, documented, and backed by working native behavior.

- Use `Lion.Engine`, idiomatic PascalCase types/members and `_camelCase` private fields. Use nullable
  annotations, deterministic builds, official analyzers and warnings as errors. Prefer simple sealed
  wrappers and readonly values to speculative service/interface hierarchies.
- Use `System.Numerics.Vector2` for Lion's actual 2D Transform. Position is local pixels, scale is local
  dimensionless factors, rotation is degrees. State explicit world/local semantics; do not expose a
  redundant Vector3 or quaternion Transform for a 2D-only engine.
- Entity is a value representing an existing native lifetime. Its default value is invalid.
  `IsValid` is a nonthrowing lifetime query on the main thread; operations on stale references throw
  an informative InvalidOperationException. Never expose pointers, IntPtr, numeric ABI tokens,
  native layout types or P/Invoke in public gameplay APIs.
- Behaviour is attached by the native runtime, not by user constructors. Entity/Transform access in
  a constructor is invalid. Match Lion's actual Awake/update passes/Destroy contract. Do not promise
  Unity lifecycle behavior that the native engine does not implement.
- Keep native component bindings separate from managed gameplay Behaviour types. Match existing Lion
  names. Cache or return lightweight values; do not allocate a wrapper each time a property is read.
- Expose a whole TransformState read/write for code that changes several fields. Ordinary properties
  remain convenient, with their boundary cost documented. Never reinterpret GLM or STL as managed
  objects. Validate finite values before writing native Transform state.
- Intern input actions once and reuse InputAction values. Reuse the native action map rather than
  adding a second binding/remapping system. Avoid per-frame string encoding, LINQ, reflection, boxing,
  temporary delegates and closures in the SDK.
- Keep Time callback-scoped and measured in seconds. Document main-thread restrictions on every
  family of operations. Detect thread misuse before touching engine state.
- Public methods need XML documentation for lifecycle, units, ownership, errors and performance where
  relevant. Do not add APIs, example calls or metadata attributes whose behavior is only planned.
- Exceptions stop at the managed bootstrap. Report script type, native owner, scene context, callback
  and full exception details. Fault isolation must not hide failures from tools or leak instances.
- Reflection belongs to cached assembly discovery/metadata generation. Fields must ultimately be
  described once through the engine's reflection/serialization boundary, with stable saved names.
- Gameplay API additions require lifetime/behavior tests against the native core. Benchmark actual
  hot paths after warmup, track allocation, and keep source/build artifacts under `Build/`.
- Keep C++ gameplay working and optional. Do not migrate existing games, rename native registration
  identifiers, or make native project compilation depend on the managed SDK.
