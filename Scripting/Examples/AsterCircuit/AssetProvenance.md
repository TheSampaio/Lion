# Asset provenance

## Original sprite atlas

`Assets/Sprites/CircuitAtlas.png` was created with the built-in image-generation tool, not copied
from a game. The user-supplied NES screenshots supplied genre/movement context, not source pixels.
The result is a 1254×1254 transparent RGBA atlas with normalized eight-column/eight-row addressing.
It is used unedited. Production refinement and a human originality/branding review remain release tasks.

Generation output:
`C:/Users/Kellvyn/.codex/generated_images/01a10755-e43f-7682-bbf6-562c53ca405f/exec-1ecd3a2f-99df-4216-b667-515d63f5c76e.png`

Exact generation prompt (the tool returned 1254×1254 rather than the requested 1024×1024):

```text
Use case: stylized-concept. Asset type: production pixel-art sprite atlas for an ORIGINAL NES-inspired side-scrolling action platform game named Aster Circuit. Generate a single 1024x1024 RGBA transparent sprite sheet, exactly 8 columns x 8 rows, each cell 128x128; artwork within each cell must look like a crisp original 32x32 pixel sprite enlarged 4x nearest-neighbor. ALL cells aligned, no labels, no text, no grid lines. Rows top to bottom: row 0 eight animation frames of one ORIGINAL amber/ivory robot courier with a rectangular dark visor and scarf, facing right (idle, run1, run2, run3, jump, shooting, hurt, victory); row 1 eight small enemy robots (crawler1, crawler2, flyingdrone1, flyingdrone2, turret1, turret2, sentry1, sentry2); row 2 eight frames of a broad copper forge boss with furnace belly and hammer arms; row 3 eight frames of a tall white/violet ice boss with crystalline shield; row 4 eight frames of an angular emerald storm boss with turbine wings; row 5 eight seamless full-cell terrain blocks in order (coppermetal, moltenrock, icebrick, snowmetal, greenindustrial, stormmetal, ladder, hazardstripes); row 6 eight pickups and hazards in order (energycapsule, healthcapsule, checkpointbeacon, spikes, playerbolt, enemybolt, flame, iceorb); row 7 eight explosion/impact frames. Characters centered, feet at cell bottom inset 8 pixels, no overlap between cells, transparent space around characters. Terrain cells full opaque squares except ladder. Limited coherent 8-bit palettes, hard pixel clusters, high readability, dark outlines, no blur, no gradients, no anti-aliasing. Entirely original designs: do NOT depict Mega Man, his helmet or arm cannon, any existing game characters, logos, or copied tiles. Reference screenshots are genre/movement inspiration only, not subjects to reproduce.
```

## Original music and effects

All ten WAV files are reproducible from `Scripting/Tools/ScoreComposer/Program.cs`. The melodies,
rhythms and oscillator parameters are authored there; no samples, soundfonts, recordings or existing
game melodies are imported. Forge, Frost and Storm have distinct note sequences, pulse duty cycles,
roots and tempos. Menu and Boss are separate compositions. Shot, Hit, Jump, Pickup and Clear are
synthesized effects. Format: mono 22050 Hz, PCM 16-bit. Regenerate from the repository root:

```powershell
dotnet run --project Scripting/Tools/ScoreComposer/ScoreComposer.csproj -- Scripting/Examples/AsterCircuit/Assets/Sounds
```

## Existing repository assets

Arcade.lnfont/Arcade.png reuse the repository-owned bitmap font from StarCollector. They are not
newly AI-generated assets. The player icon and mandatory startup splash are existing Lion assets.
The normal exporter gathers engine, native dependency and .NET license notices. No third-party
commercial-release clearance is implied by this provenance record.
