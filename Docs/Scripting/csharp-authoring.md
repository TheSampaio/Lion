# Author C# gameplay in Mane

## Requirements and current scope

Use Windows x64 and the .NET 10 SDK for C# authoring. Building Lion itself or native gameplay also
requires Visual Studio C++ tools; C#-only projects use the supplied scene-player module. Its SDK
packaging also produces `Managed/Lion.Engine.dll`, runtime configuration and XML documentation
beside Mane when the optional .NET SDK is installed. Native C++ projects do not require .NET.

Editor gameplay and Windows player export are supported. Export builds Shipping scripts, seals
assets and packages an app-local .NET runtime plus its mandatory licenses. The player does not need
a .NET installation or SDK. Native Visual C++ runtime prerequisites still apply. Brickout remains C++.

## Create and run a script

1. Create or open a Lion project in Mane.
2. Select an entity, open **Add Component**, then **New Component**. Choose C#, enter a class name
   and an Assets-relative folder, then Create. Mane writes a `.cs` file and starts Compile.
3. After compilation succeeds, add the new `Game.YourClass` from Add Component. Ordinary C# code
   depends only on `Lion.Engine`, not the native core's headers or interop internals.
4. Edit fields marked `[Editable]` in Properties and save the scene with Ctrl+S.
5. Play runs the script against the native scene. Pause and Step use the same scene timestep;
   Stop restores the exact authored snapshot, including transforms and field values.
6. After changing C# code, Compile (Ctrl+Shift+B) rebuilds and reloads the catalog. Recompilation
   replaces a private managed build copy, not the assembly currently in use.

The generated `Build/lion-scripts.csproj` is openable in a C# IDE for IntelliSense. It references the
installed SDK beside Mane and includes `.cs` files under Assets. Regeneration updates that SDK path
when the project is moved between installations. All generated artifacts remain under Build.
C#-only projects copy the packaged scene-player module instead of invoking a C++ compiler. Projects
with authored native components still build through their solution. The native bootstrap is engine
tooling; normal C# gameplay does not require changing it.

```csharp
using System.Numerics;
using Lion.Engine;

namespace Game;

public sealed class Mover : Behaviour
{
    [Editable] public float Speed = 100;
    [Editable] public Vector2 Direction = Vector2.UnitX;

    public override void OnUpdate(float deltaTime)
    {
        Transform.Position += Direction * Speed * deltaTime;
    }
}
```

Only marked mutable instance fields participate. Supported types are float, int, bool, string and
Vector2; private and inherited fields work too. The exact field name is the persistent scene key.
Missing keys retain declared defaults. Renamed fields have new keys. Incompatible saved types report
an error and reset that script's fields to defaults; explicit migration tooling is not implemented.
Properties, readonly/static fields, enums, lists, entity/asset references and range/tooltips are not
supported yet. Null strings serialize as empty strings.

Keep constructors free of gameplay, Entity access, resource ownership and event subscriptions:
Mane constructs a temporary unattached instance to obtain defaults. Initialize runtime behavior in
OnAwake and clean up in OnDestroy. Gameplay callbacks remain dormant while editing.

Compiler diagnostics and script exceptions appear in the native Console. A compiler failure leaves
the existing catalog in use. If a compilable new assembly has invalid metadata, Mane attempts to
restore the last valid private catalog for that same project before reconstructing its scene. With
no previous catalog available, fix compilation/discovery before authoring or saving script components.

## Verification and CI

`Scripts/VerifyCSharp.ps1 <configuration>` exercises real native scenes, serialization, callbacks,
handles, exceptions and allocation. `Scripts/VerifyCSharpEditor.ps1 <configuration>` creates a fresh
copy of the editor fixture under Build, compiles and exports it through Mane, then executes gameplay
with system .NET discovery disabled. The report checks actual app-local framework and script paths.
Open its printed project path to inspect the five supported field types and oscillating sprite.

The editor also supports `Lion.exe --compile-project <absolute-project-directory>` without opening
a UI, using the configuration of that editor executable. A Shipping editor is a WindowedApp: CI
must wait for process termination and inspect its exit code, not assume console-shell waiting.

For a complete C# game and the supported scene/rendering bindings, see the
[gameplay guide](csharp-gameplay.md). Physics/audio/UI bindings and richer serialization remain
future increments; see the architecture roadmap.
