# Aster Circuit

An original NES-inspired action platformer and a C# gameplay stress project for Lion 0.44.0.
The amber/ivory courier restores three corrupted industrial signals. No existing game's character,
sprite, tile, recording or melody is imported. This is a playable production-direction prototype,
not a certified commercial release.

| Stage | Warden | Theme / soundtrack | Reward |
| --- | --- | --- | --- |
| Pyre Foundry | Crucible | Copper furnaces, heat vents, 144 BPM pulse score | Ember Fan |
| Glacier Vault | Rime Warden | Ice inertia, ladders, crystalline shield, 112 BPM score | Rime Lance |
| Tempest Array | Volt Kestrel | Wind, wider gaps, aerial attacks, 168 BPM score | Arc Wave |

Title/menu, stage select, three stages/bosses, pickups, checkpoints, three lives, retry/game-over,
pause, weapon unlocks, saved clears and a final ending are implemented in Assets/Scripts.
Native Lion owns scenes, physics/colliders, sprite/text batches, widgets, audio and particles.
There is no authored C++ game module. All scene/Assembly identities are resource-relative.

## Open, build and export

Open this directory in Mane, Compile, then Play. The .NET 10 SDK is the scripting authoring
prerequisite; C#-only projects use the packaged native scene-player bootstrap. For automation:

```powershell
Build\Bin\Debug\Mane\Lion.exe --compile-project Scripting\Examples\AsterCircuit
Build\Bin\Shipping\Mane\Lion.exe --export-windows Scripting\Examples\AsterCircuit Build\AsterCircuitExport
Scripts\VerifyAsterCircuit.ps1 -PlayerDirectory 'Build\AsterCircuitExport\Aster Circuit'
```

Wait for Mane's exit and check its exit code. The export includes app-local .NET, sealed assets and
licenses, not source files or PDBs. Build/exports are local artifacts and stay out of Git.
`AsterCircuit.Game.csproj` also compiles client code directly against the source SDK for verification;
Mane owns the real generated project under Build/.

## Controls

- Arrows / WASD: move and climb ladders. Z / Space: jump (hold for a higher jump).
- X / J: fire. C: cycle unlocked weapons. Special weapons consume energy; pulse remains available.
- P: pause. Escape: stage select / title / quit. Enter: confirm. Mouse: native menu buttons.
- Gamepad bindings: left stick / D-pad, A jump/confirm, X or B fire, right bumper weapon, Start pause.

Progress is stored in `%LOCALAPPDATA%/LionGames/AsterCircuit/Progress.lnsave`. New Campaign clears
restored signals. Checkpoints apply to current-stage retries; saved clears unlock weapons across runs.
`ASTER_SAVE_DIRECTORY` isolates automated test progress. Optional `ASTER_TEST_REPORT` writes observed
state for the owned-window verifier; it never teleports, grants damage immunity or clears a boss.

## Source map

FrontEnd owns menus/selection; StageController composes the world and pools bolts/impacts; Courier
uses native velocity/contact/raycast; Warden implements three enemy types and distinct boss patterns;
Art configures native rendering/HUD; GameSession owns progression. ScoreComposer generates all WAVs
deterministically. See [asset provenance](AssetProvenance.md), [SDK coverage](../../../Docs/Scripting/csharp-parity.md)
and [verification/release boundaries](../../../Docs/Scripting/aster-verification.md).
