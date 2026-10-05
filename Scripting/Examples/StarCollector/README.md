# Star Collector

A complete small scene-authored game with C# gameplay only: menu, input, movement, collection,
countdown, HUD, win/loss, restart and quit. No C++ source or developer-owned native entry point.

Open this directory as a Lion project, Compile, then Play. Export Windows builds Shipping scripts
and packages the .NET runtime beside the player. On the player machine no SDK or .NET installation
is required. The precompiled scene-player bootstrap is engine tooling, not gameplay; C# project
compilation needs the .NET 10 SDK but no C++ compiler. Native Visual C++ runtime prerequisites apply.

Controls: Enter/Space starts or retries, WASD/arrows move, Escape quits (or stops Mane's Play).
The three collectibles are deliberately in a line so the complete loop is easy to verify.

Use the editor to author camera, sprite and text traits. `PlayerController` caches native input
actions and a typed Sprite Renderer. `GameController` resolves the authored scene once, updates
text only when the score/second changes and uses deferred native entity removal. `MenuController`
requests scene transitions from the update boundary. This is not a replacement physics system;
the collectible's distance threshold is this game's pickup rule.

The bitmap font and sprite art reuse the repository's native demo assets. Scene/input documents are
readable fixtures; editor saves seal them, and Shipping seals all packaged assets.

See [the C# gameplay guide](../../../Docs/Scripting/csharp-gameplay.md) for API contracts and limits.
