# C# player increment verification

Verified on Windows x64 on October 5, 2026 for Lion 0.43.0. Artifacts and screenshots are local
build output, not repository assets.

- Debug, Release and Shipping full native builds succeeded. Existing native CRT linker/deprecation
  warnings remain; managed SDK/example/test builds produced zero warnings and errors.
- Native integration passed in all three configurations: lifecycle, authoring/serialization,
  exception isolation, thread rejection, UTF-8 bindings, deferred removal, removed-trait views and
  scene invalidation. The binding fixture loads through SceneSerializer with its script before the
  entity it resolves, covering complete-scene availability during Awake.
- The allocation probe includes Transform, cached input and HasComponent. It reports no managed
  allocations after warmup. One final Release run measured 12.26 ms native versus 83.66 ms hosted
  for 256 entities over 1000 updates; Shipping measured 11.13 ms versus 83.82 ms. These are local
  single-run integration measurements, not controlled performance claims.
- Mane compiled and exported fresh C# fixtures in all configurations. Players ran from an unrelated
  working directory with invalid system DOTNET_ROOT values and minimal PATH. Reports confirmed
  native Transform changes, the package's System.Private.CoreLib and its compiled script assembly.
  Missing managed SDK startup is checked separately and must return nonzero.
- Star Collector compiled with Visual Studio discovery disabled, using the packaged Player module.
  Its Shipping export was run and captured through menu, movement/collection, victory, restart,
  timed defeat, retry and successful quit. No C++ gameplay source is present in this project.
- Mane was run in all configurations. Authoring, Play, C# scene transition and Stop were inspected;
  Stop restored the authored menu after transitioning into the gameplay scene.
- The native launchers were run and inspected in all configurations. The Sandbox was exported and
  run separately. Both native and C# exported players show only the new engine-owned splash, fading
  in and out before the menu; the old Splash scene/component are no longer shipped from the project.

Reproduce automated checks with `Scripts/Build.bat`, `Scripts/VerifyCSharp.ps1` and
`Scripts/VerifyCSharpEditor.ps1` for each configuration. See the [gameplay guide](csharp-gameplay.md)
for implementation boundaries. This does not certify a clean Windows VM, every native engine
binding, collectible-context garbage collection or state-preserving gameplay hot reload.
