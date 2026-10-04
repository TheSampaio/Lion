# Lion C# gameplay

This SDK hosts real gameplay in the native engine. It is opt-in and does not migrate
Brickout. Mane can now scaffold, compile, attach and configure C# scripts, serialize their fields,
and run them in Play mode. Managed player export is not implemented yet and is blocked explicitly.
See [the authoring guide](../docs/scripting/csharp-authoring.md),
[the architecture and roadmap](../docs/scripting/csharp-architecture.md)
and [API conventions](../docs/scripting/csharp-api-guidelines.md).

## Build and verify

Requirements: Windows x64, the .NET 10 SDK, Visual Studio C++ tools, and the existing recursive
Lion checkout. Native projects still build without .NET.

```powershell
Scripts\Build.bat Debug
Scripts\VerifyCSharp.ps1 Debug

Scripts\Build.bat Release
Scripts\VerifyCSharp.ps1 Release

Scripts\Build.bat Shipping
Scripts\VerifyCSharp.ps1 Shipping

# Compile a generated native bootstrap and C# gameplay through Mane, then verify the export guard.
Scripts\VerifyCSharpEditor.ps1 Debug
```

The verifier builds the documented SDK and actual example/test scripts, compiles a native runner
against the packaged Lion SDK, then hosts .NET inside that runner. All artifacts stay in
`Build/Managed`. No external test framework or NuGet package is needed.

Checks include real Transform writes, callback ordering, scene timesteps, hidden/disabled/paused
owners, component disabling, worker-thread rejection, stale references retained by C++ callers,
owner reattachment, constructor/update exceptions, cleanup, runtime restart, editable field metadata,
authoring/Play/Stop serialization, reentrant Inspector edits, and warmed-up allocation
measurements. Expected exception diagnostics are part of the tests. The process returns nonzero
for any failed check. The benchmark compares 256 native/managed entities over 1000 real scene updates;
it is a foundation-level measurement, not a stable cross-machine performance claim.

## Gameplay example

The compiled [Mover](Examples/Mover.cs) uses the actual implemented API:

```csharp
using System.Numerics;
using Lion.Engine;

public sealed class Mover : Behaviour
{
    public override void OnUpdate(float deltaTime)
    {
        var state = Transform.State;
        state.Position += new Vector2(100.0f * deltaTime, 0.0f);
        Transform.State = state;
    }
}
```

`Transform.Position += ...` also works. Changing several fields through `State` performs one
snapshot read and one write, with no per-frame wrapper allocation. System.Numerics handles the math;
native Lion owns the Entity, Transform, scene loop, and renderer.

For the current opt-in native harness, include `<Lion/Scripting/CSharpRuntime.h>` and
`<Lion/Scripting/CSharpScript.h>`, call `CSharpRuntime::Initialize` with an absolute built SDK directory
and hostfxr path, then `LoadAssembly` with the game's assembly path. Attach a
`CSharpScript("Namespace.TypeName")` before adding its owner to a Scene. The regular native Scene
lifecycle drives the script. Clear scenes and call `Shutdown` before stopping the host. Tooling will
eventually own these steps; game developers should not need to edit C++ for ordinary authoring.

Input names are resolved once using `Input.Action("Move")` and read through the cached value's
`Strength`. The first binding mirrors native signed strength, not a newly invented axis system.
Log messages go through native verbosity, and `Time.DeltaTime` refers to the active update callback.
The generated `Lion.Engine.xml` documents the public API alongside `Lion.Engine.dll`. The editor
receives these under `Managed/` when built with the .NET 10 SDK installed.
