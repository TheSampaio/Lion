#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Lion/Lion.h>

// What both faces of the editor know about projects: the Project Manager window that greets, and the
// editor that opens one. A project is a folder holding a game's Assets and Source; a scaffolded one also
// carries a .lproject marker naming it. This lives once, here, because two windows that disagree about
// what a project is would send a person to two different games.
namespace Projects
{
	// The marker a scaffolded project carries; its stem is the project's name.
	constexpr const Lion::char8* kFileExtension = ".lproject";

	// The engine's own tree, found by walking up from the executable's directory: the editor runs from its
	// build output, several folders below it. Anchored to the executable, not the working directory — a
	// file dialog moves that out from under the process. Empty when the tree is not around (a distributed
	// editor), which is what disables generating and compiling.
	std::filesystem::path EngineRootDirectory();

	// The built-in Sandbox inside that tree — the project the editor falls back on, and the one it can
	// always offer.
	std::filesystem::path DefaultProjectDirectory();

	// The editor's persisted state (recents, layouts, shortcuts), beside the executable — not in whatever
	// directory the editor happened to be started from.
	std::filesystem::path EditorDataDirectory();

	// A folder the editor can open as a project: one that holds a game's assets. A scaffolded project also
	// carries its marker, but a plain Assets folder is enough to browse and edit.
	bool IsProjectFolder(const std::filesystem::path& folder);

	// The project's name: the marker's stem, or the folder's own name when it has none (the Sandbox).
	std::string DisplayName(const std::filesystem::path& project);

	// The engine version its marker records, or nothing when it records none — the manager's list shows it
	// the way Godot's shows which engine a project was made with.
	std::string EngineVersion(const std::filesystem::path& project);

	// The recently-opened projects, newest first, dead entries dropped — with the built-in Sandbox appended
	// when absent: it is the one project never opened through the manager, and it must stay reachable.
	std::vector<std::string> LoadRecent();

	// Puts the folder at the front of the recents, once, capped — a project opened again is the most
	// recent, not a second entry.
	void Remember(const std::filesystem::path& folder);

	// Scaffolds <location>/<name>: the marker, the Assets and Source folders, and a game module complete
	// enough to compile from the first build. Returns the project's folder, or an empty path with 'error'
	// saying why not.
	std::filesystem::path CreateOnDisk(const std::string& name, const std::filesystem::path& location, std::string& error);
}
