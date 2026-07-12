#pragma once

namespace Lion
{
	// Names of the game module, fixed by the engine rather than by whatever the game project happens
	// to be called: the launcher and the editor both load it by these names, so they cannot drift.

	// The module itself, sitting next to the executable that loads it.
	constexpr const char8* kGameModuleFile = "lion-game.dll";

	// The editor loads a *copy* under this name: Windows locks a loaded library, so leaving the
	// original writable is what lets the module be rebuilt while the editor is still running.
	constexpr const char8* kGameModuleLoadedFile = "lion-game.loaded.dll";

	// The module's symbols, and the copy the editor points its own copy of the module at. A debugger
	// holds a loaded module's PDB open just as Windows holds the library itself, so the copy needs one
	// of its own for the same reason — otherwise the next build cannot write the file the linker owns.
	constexpr const char8* kGameModuleSymbolsFile = "lion-game.pdb";
	constexpr const char8* kGameModuleLoadedSymbolsFile = "lion-game.loaded.pdb";

	// The symbol a game module exports to hand back its Application (see Game/GameModule.cpp).
	constexpr const char8* kGameModuleEntryPoint = "LionCreateApplication";
}
