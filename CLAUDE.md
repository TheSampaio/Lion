# Lion Engine — working agreement

A 2D C++ game engine.

**A folder is never named after its project**, so a path always says which of the two it means.

| Folder | VS project | Group | Output | What it is |
|---|---|---|---|---|
| `Engine/` | `Lion` | Core | `lion-core.dll` | The engine. |
| `Sandbox/` | `Game` | Runtime | `lion-game.dll` | The game's code, as a module. Its components live in `Sandbox/Assets/Scripts/`. |
| `Editor/` | `Mane` | Tools | `Lion.exe` | The editor — the face of the engine, so it carries its name. |
| `Launcher/` | `Launcher` | Tools | `lion-launcher.exe` | Thin exe: loads the module and runs it. Owns no game code. |

The module's file names are fixed in `Lion/Core/GameModule.h`, not by the project name — the loaders
must not care what a game calls itself. `lion-game.loaded.dll` is the editor's private copy of the
module (see hot reload, below).

## Language

- **Code, comments, commit messages, PRs: English.** No exceptions, including identifiers.
- **Conversation with me: Portuguese.**

## Git

- **Always commit, then push to `origin/dev`.** Never merge, and never touch `main` — the user opens
  and merges the PRs.
- **Bump `kVersion` (`Lion/Core/Version.h`) with the change**, before committing it: the patch for a
  fix, the minor for something the engine can now do, the major for something it does differently
  enough to break a game written against the old one. The editor's status bar reads it, so a version
  that never moves is a version nobody believes.
- **Only commit what is verified working.** For anything visual or behavioural that means running it
  and looking at it (screenshot), not just a green build. A build that compiles proves nothing about
  whether the feature works.
- Never commit changes the user made to their own working tree without asking. Stage your own files.
- **What the user makes with the engine is not the engine.** A component scaffolded from the editor
  (`Sandbox/Assets/Scripts/`) belongs to whoever is building a game with it — it is the editor working,
  not the engine changing. Leave those files alone unless the user asks for them, even when they are the
  only thing standing between the tree and a clean `git status`. The Brickout demo's own components are
  the exception, because the sandbox *is* part of this repository.

## Design principles

Applied to every change, not just new code:

- **Clean Code** — names that state intent, small functions that do one thing, no surprises.
- **KISS** — the simplest thing that actually solves the problem. Resist speculative generality.
- **DRY** — one source of truth. A second copy of a rule is a bug waiting to diverge: the shortcut
  leak happened because the viewport read its own tool keys instead of going through
  `HandleShortcuts`, so hardening one path did nothing for the other.
- **SOLID** — in particular, depend on abstractions at boundaries. `Component::Serialize` takes an
  abstract `Serializer` precisely so the JSON library never crosses into a component's translation
  unit, and therefore never crosses the DLL boundary.
- **Performance is a feature.** Pass by `const&`, reserve buffers, avoid per-frame allocation. The
  console renders through an `ImGuiListClipper` because submitting a full history every frame was
  visibly costing FPS.

## C++ conventions

Follow the surrounding code; it is consistent. In short:

- **Naming.** Types and functions `PascalCase`; locals and parameters `camelCase`; members `mCamelCase`;
  constants `kPascalCase`; macros `LION_UPPER_CASE`.
- **Formatting.** Allman braces; no braces for a single-statement `if`. Indent with tabs — that is what
  most of the tree uses, though a minority of older files are space-indented, so match the file you
  are editing rather than reformatting it.
- **Types.** Use the engine aliases: `int32`, `float32`, `char8`, `Scope<T>` (unique), `Reference<T>`
  (shared), `MakeScope` / `MakeReference`.
- **Headers.** `#pragma once`. Public engine headers export with `LION_API` and stay free of
  third-party types — forward-declare instead of including where you can.
- **Comments explain *why*, never *what*.** The code already says what it does. Document the
  constraint, the trade-off, or the non-obvious reason a thing is done that way. Doc comments follow
  the Google C++ style. Never write a comment that talks to the reviewer ("changed this to fix X") —
  it is noise the moment the PR merges.

## Build

This project's artefacts land under `Build/`. Each vendored library builds inside its own folder
(`Vendor/<lib>/Build/`), so a library's output never mixes with the engine's — set from the workspace
script, not by editing the submodules.

**Every dependency is a submodule**, each a fork under `github.com/TheSampaio`, each carrying its own
`premake5.lua`. Nothing is copied into the tree by hand, so `git clone --recursive` is all a fresh
machine needs. A vendored licence travels with whatever the build copies out of it — the icon font
lands beside the editor with its `LICENSE` next to it.

```sh
Scripts\Build.bat [Debug|Release|Shipping]   # from a fresh clone: generates, then builds everything
```

The script does not order anything: each project declares what it depends on, so MSBuild works out
that the libraries come before the engine, which comes before the game module, which comes before the
tools that load it. Post-build steps create the directories they copy into.

By hand:

```sh
premake5 vs2022                                                       # after any file added/removed
MSBuild Lion.sln -p:Configuration=Debug -p:Platform=x64 -m            # Debug, Release, Shipping
MSBuild Lion.sln -t:Runtime\Game -p:Configuration=Debug -p:Platform=x64  # only the game module
```

Projects are generated by premake, so **the file list is a glob**: adding or deleting a source file
requires regenerating before building, or the `.vcxproj` still points at the old set.

An MSBuild target for a single project is `<solution folder>\<project>`, so **moving a project between
groups renames its target** — the editor's Compile builds `Runtime\Game` and would break silently.

**Verify against all three configurations.** They are not variations on a theme; each one exists to
catch something the others cannot:

| | Optimised | Symbols | The game logs | Why it exists |
|---|---|---|---|---|
| **Debug** | no | yes | everything | Day-to-day work. |
| **Release** | yes | no | Error/Warning/Success | Measuring performance with the engine still talking. |
| **Shipping** | yes | no | nothing | What reaches a player. Changes the entry point too. |

Release is not "Debug but faster": it is optimised, which surfaces what Debug hides. Building a
configuration is not verifying it — run it.

**Those levels are the game's.** The editor is a tool, so it logs everything in every configuration —
it asks for it in `Lion::Main` before the engine starts. That is why the log's filter is a runtime
verbosity and not a `#ifdef`: the engine is one DLL shared by both, so a compile-time switch would
silence the editor along with the game. For the same reason, a call site must not add a guard of its
own — `Log::IsEnabled` is there to skip building a message, not to decide policy twice.

Run the editor from `Build/Bin/<config>/Mane/Lion.exe` and the game from
`Build/Bin/<config>/Launcher/lion-launcher.exe`. Both anchor their assets, the game module and the
editor's own state to the executable, not to the working directory.

## Architecture notes worth knowing

- **Sealing lives in one place: `Lion/Core/Vault.h`** — XOR against `0x07D2`, then URL-safe base64. A
  scene is sealed by the editor that saves it; a shipped game's shaders are sealed by the build, which
  runs the editor to do it (`Lion.exe --seal <dir> .glsl .lscene`, from the Launcher's Shipping
  post-build — `Editor/Source/Sealer.h`). The rule used to live a second time in a PowerShell script,
  and a third time as a whole project, which is two copies of a rule too many. Loading never has to know
  which kind it has: `Vault::Unseal` gives plain content back unchanged, and sealed content is base64 and
  nothing else, so a `#` or a `{` answers the question. It is obfuscation, not encryption — the key ships
  with the binary, and a key you ship is a key you gave away.

- **An entity is composed, never derived.** `Entity` is `final`: a name, a `Transform`, and the
  components attached to it. Everything else — including collision, through `Component::OnCollision` —
  is a component, because a trait added by subclassing is one the editor cannot list, the scene cannot
  save and another entity cannot reuse. A component derives from `Component` and nothing else; one
  that needs another asks its owner (`GetOwner()`), or the scene, for the trait rather than for the
  object.
- **The game is a module.** Components register themselves by name (`LION_REGISTER_COMPONENT`), which
  is what lets the editor list, create and serialize a type it was never compiled against. Loading the
  DLL runs those static initializers — that is the whole mechanism. `Lion::LoadGameModule` /
  `UnloadGameModule` own the bookkeeping, so neither loader can forget half of it.
- **Hot reload is delicate.** Windows locks a loaded library, so the editor loads a *copy* and leaves
  the original writable. The registries' factories are code inside the module, so they are dropped
  before it is unloaded. Live components have their vtables in the module, so the scene round-trips
  through its serialized form and nothing may hold a stray `Reference<Entity>` across the swap.
- **The editor and the game share one `Lion.dll`,** which is why a registry populated from the game
  module is visible to the editor. Every module uses the shared (`/MD`) runtime, so cross-module
  `new`/`delete` is safe.
- **`%{wks.location}` becomes `$(SolutionDir)`.** Building a `.vcxproj` directly (outside the solution)
  therefore resolves it to the project directory and breaks the post-build paths. Build through the
  solution.
