# Gameplay SDK and Aster Circuit verification — 0.44.0

Verified locally on Windows x64 on October 5, 2026. This is a functional prototype verification,
not commercial certification or literal parity with every C++ header. See [coverage](csharp-parity.md)
and [asset provenance](../../Scripting/Examples/AsterCircuit/AssetProvenance.md).

## Engine and SDK

- Full Debug, Release and Shipping builds passed. Managed builds reported zero warnings/errors.
- `Scripts/VerifyCSharp.ps1` passed in all three configurations using real native scenes and .NET hosting.
  Checks cover ABI/layout, lifecycle/render/contact callbacks, exception isolation, thread/lifetime
  guards, authoring fields, dynamic composition, deferred removal and failed-initializer rollback,
  hierarchy/world transforms, native physics/raycast, shared reflected fields and widget state.
- The physics probe and warmed-up update benchmark reported no per-thread managed allocations.
  The final Release benchmark measured native 11.8843 ms / hosted C# 91.3258 ms; Shipping measured
  11.8954 ms / 92.1256 ms for 256 entities over 1000 scene updates. These are individual local
  measurements, not equal-performance claims or statistically controlled benchmarks.
- Mane was run and visually inspected in Debug, Release and Shipping: project Compile, dormant
  authoring, F5 Play, F8 Stop and clean shutdown passed, with no runtime errors in the captured logs.
- The native Brickout launcher was run outside its installation directory in all three configurations.
  Its splash and menu were captured and inspected. The unrelated authored MainMenu edit was preserved.
- Editor-project compilation/export and app-local managed-player tests passed in all three
  configurations. The Shipping check resolved System.Private.CoreLib from the packaged Dotnet
  directory; removing the packaged SDK correctly produced a failing exit code.

## Exported game

Mane's real `--export-windows` path produced the Shipping player, not a manually assembled distribution.
The package includes matching native binaries, compiled managed assemblies, app-local .NET 10.0.12,
sealed assets and license notices. It contains no C#/C++ source, headers or PDB files.

`Scripts/VerifyAsterCircuit.ps1` sends keyboard messages only to its owned player window. Progress and
reports are isolated under Build. Reports only observe gameplay; there is no test teleport, damage
immunity, boss-clear command or edited completion mask. Briefly empty/truncated report reads are retried.

The full campaign passed through normal movement, jumping, ladder climbing, shooting and weapon selection:
Pyre Foundry/Crucible, Glacier Vault/Rime Warden and Tempest Array/Volt Kestrel all cleared, producing
Cleared=7, the ending screen, return to title and a successful player exit. The same exported player also
passed all-stage movement/jump/fire, audio initialization, pause/frozen physics and scene return checks.
Normal falls exercised life loss, Game Over and retry. A separate process loaded the campaign-produced
save, restored all three clears and cycled all three unlocked weapons back to pulse.

Visual inspection included the new engine splash during fade-in/hold/fade-out, title and stage select,
the three themes, HUD/pause, boss combat, Game Over/retry and ending. The old placeholder splash is not
part of this game's startup path. Gameplay testing caught and corrected per-tile collider seam snagging,
ladder dismount geometry, platforms obstructing storm gaps, jump reach and falls being delayed by
contact invulnerability.

Five original scores and five synthesized effects were decoded/played by native audio. PCM inspection
confirmed mono 22050 Hz, 16-bit data, nonzero signal and no clipping. Stage tracks have distinct tempos
and melodies. This does not replace human auditory review.

Local evidence and the final package are under `Build/Verification/AsterFinal/`; these are ignored build
artifacts, not repository content. Repeat on a fresh checkout:

```powershell
Scripts/Build.bat Debug
Scripts/Build.bat Release
Scripts/Build.bat Shipping
Scripts/VerifyCSharp.ps1 Debug
Scripts/VerifyCSharp.ps1 Release
Scripts/VerifyCSharp.ps1 Shipping
Scripts/VerifyCSharpEditor.ps1 Shipping
Build/Bin/Shipping/Mane/Lion.exe --export-windows Scripting/Examples/AsterCircuit Build/AsterCircuitExport
Scripts/VerifyAsterCircuit.ps1 -PlayerDirectory 'Build/AsterCircuitExport/Aster Circuit' -Campaign -FailurePaths
```

For a persistence check, pass `-ProgressDirectory` pointing to the successful campaign's Save directory,
`-ExpectedCleared 7 -FailurePaths` and a fresh OutputDirectory, without `-Campaign`.

## Remaining release work

Human playtesting/balance, gamepad and mouse interaction QA, accessibility/remappable controls,
art-atlas cleanup, listening/mix/loop refinement, originality/branding review and clean-Windows-machine
distribution testing remain. Small generation artifacts around some projectile cells are known art
refinement work. The app-local framework test does not certify native prerequisites on a clean machine.
State-preserving script reload and collectible-context reclamation also remain separate SDK work.
