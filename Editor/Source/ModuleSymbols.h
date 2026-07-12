#pragma once

#include <string>

// Points a library at a different PDB than the one it was linked against.
//
// A debugger attached to the process keeps a loaded module's PDB open, which is enough to stop the
// linker from writing it — so rebuilding the game module while debugging the editor fails outright.
// Copying the library is not enough on its own: the copy still names the original's PDB, so the
// debugger locks the very file the next build has to produce.
//
// Rewriting the name in the copy is what closes that gap. Give the copy its own PDB, and the
// debugger holds *that* one while the linker's stays free.
//
// Returns false when the module records no CodeView entry (a build without symbols, which needs
// nothing done to it) or when the new name is longer than the one it replaces — the string is
// patched in place, so it can only shrink.
bool RedirectModuleSymbols(const std::string& modulePath, const std::string& symbolsFileName);
