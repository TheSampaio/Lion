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

	class DynamicLibrary;

	// Loads a game module, attributing everything it registers to it. Returns whether it loaded.
	//
	// Loading runs the module's static initializers, which is how its components register themselves.
	// That has to be recorded, because those entries are code inside the module and have to go when it
	// does: a loader that skips the bookkeeping leaves them behind to be destroyed after the library is
	// gone. It is the loading that owes this, not the loader, so neither one is free to forget it.
	LION_API bool LoadGameModule(DynamicLibrary& module, const std::string& path);

	// Drops a loaded game module together with everything that is code inside it.
	//
	// The registries hold factories that live in the module, so they go before it does: left to the
	// process to tear down, they are destroyed after the library has been unmapped — a call into memory
	// that is no longer there. Both loaders end up here, the editor when it reloads or closes and the
	// launcher when the game exits, because the rule belongs to the module and not to either of them.
	//
	// Whatever holds live instances of the module's types has to be gone by the time this is called.
	LION_API void UnloadGameModule(DynamicLibrary& module);
}
