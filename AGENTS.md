# Lion Engine Agent Guide

## Project identity

Lion is a 2D game engine written in C++20. A folder is deliberately not named after its Visual Studio
project, so paths always distinguish the source area from the build target:

| Folder | Project | Output | Responsibility |
|---|---|---|---|
| `Engine/` | `Lion` | `lion-core.dll` | Engine runtime and public API |
| `Sandbox/` | `Game` | `lion-game.dll` | Built-in Brickout game module |
| `Editor/` | `Mane` | `Lion.exe` | Lion editor |
| `Launcher/` | `Launcher` | `lion-launcher.exe` | Thin standalone game launcher |

Read `CLAUDE.md` before changing the repository. It is the detailed working agreement and remains the
source of truth when this guide is only a summary.

## Communication and naming

- Speak Portuguese with the user.
- Write code, identifiers, comments, documentation, commit messages, and PR text in English.
- Follow the existing C++ style: PascalCase types/functions, camelCase locals/parameters, `mCamelCase`
  members, `kPascalCase` constants, and `LION_UPPER_CASE` macros.
- Prefer tabs and Allman braces; match the file when editing an older space-indented file.
- Use Lion aliases (`int32`, `float32`, `char8`, `Scope`, `Reference`, `MakeScope`, `MakeReference`).
- Public engine headers use `LION_API`, avoid third-party types, and forward-declare where practical.
- Comments explain constraints and reasons. Public documentation follows Google C++ doc-comment style.

## Architecture boundaries

- Entities are composed and never subclassed. Gameplay and collision behavior belong in `Component`
  subclasses attached to a final `Entity`.
- Components register with `LION_REGISTER_COMPONENT`; registration names are the scene/editor ABI.
- Describe editable component fields once through `Reflect()`. Reflection drives both Inspector widgets
  and scene serialization through abstract `Reflector`/`Serializer` boundaries.
- The game is a dynamically loaded module. Keep component factories and live component instances from
  surviving module unload; hot reload must serialize the scene, unload, reload, and deserialize.
- Keep engine, game-module, editor, and launcher responsibilities separate. The launcher owns no game
  behavior. A project owns its generated game-module build and compiles against the packaged SDK.
- Preserve the shared-DLL and shared-runtime constraints: the editor and game module use the same
  `lion-core.dll` and `/MD` runtime.
- Keep C++-specific generation/build details behind explicit language or tooling boundaries so C# can be
  introduced later without teaching scenes, entities, components, or the Inspector about either language.
- Centralize rules. In particular, sealing belongs in `Lion/Core/Vault.h`, module filenames in
  `Lion/Core/GameModule.h`, and shortcut handling in the shared shortcut path.
- Apply Clean Code, SOLID, DRY, YAGNI, and KISS. Prefer the smallest complete design, clear ownership,
  dependency abstractions at module boundaries, and no speculative framework.
- Treat performance as a feature: pass by `const&`, reserve stable buffers, and avoid per-frame allocation
  or repeated filesystem traversal.

## Editor and assets

- Project-authored content belongs to the project, not the engine. Do not modify user-generated scripts
  unless requested; the built-in Sandbox demo is the repository-owned exception.
- Scene paths and serialized asset paths are resource-relative and use forward slashes. Never save a
  machine-specific absolute asset path.
- Editor icons use the established Material Design icon helpers and the shared 16 px / 24 px scale.
- New or removed C++ files require regenerating the Visual Studio projects because Premake file lists are
  globs captured at generation time.

## Build and verification

From the repository root:

```bat
Scripts\Build.bat Debug
Scripts\Build.bat Release
Scripts\Build.bat Shipping
```

Or regenerate with `premake5 vs2022` and build `Lion.sln` through MSBuild. Do not build a generated
`.vcxproj` directly when its paths rely on `$(SolutionDir)`.

Before committing:

1. Build Debug, Release, and Shipping.
2. Run the editor from `Build/Bin/<config>/Mane/Lion.exe` in all three configurations.
3. Run the game from `Build/Bin/<config>/Launcher/lion-launcher.exe` in all three configurations.
4. Exercise changed behavior and visually inspect visual/editor changes; compilation alone is insufficient.
5. Check `git diff` and stage only files belonging to the requested change.

## Git delivery

- Work on `dev`; never merge and never modify `main`.
- Preserve unrelated and user-created working-tree changes.
- Bump `Engine/Source/Lion/Core/Version.h` before committing: patch for a fix, minor for a new capability,
  major for a breaking behavior change.
- Commit only verified work, using the repository's concise imperative English subject style.
- Push the verified commit to `origin/dev`. The user opens and merges the PR.
