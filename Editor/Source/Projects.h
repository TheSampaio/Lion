#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Lion/Lion.h>

// What both faces of the editor know about projects: the Project Manager window that greets, and the
// editor that opens one. A project is a folder holding a game's Assets and Source; a scaffolded one also
// carries a .lnproject marker naming it. This lives once, here, because two windows that disagree about
// what a project is would send a person to two different games.
namespace Projects
{
	// The marker a scaffolded project carries; its stem is the project's name.
	constexpr const Lion::char8* kFileExtension = ".lnproject";

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

	// One row of the Project Manager's list: everything the window shows about a project, read once per
	// refresh rather than per frame.
	struct Entry
	{
		std::string path;
		std::string name;
		std::string version;   // The engine that made it; the running engine's for the built-in.
		std::string edited;    // Last edited, "YYYY-MM-DD HH:MM"; empty when the folder is gone.
		std::filesystem::file_time_type editedAt = {};
		bool missing = false;  // Still on the list, no longer on disk — what Remove Missing sweeps.
		bool favorite = false;
		bool builtIn = false;
	};

	// Every project the manager lists: the recents as written — the missing ones included, greyed rather
	// than hidden, so Remove Missing has something to act on — with the built-in appended when absent.
	std::vector<Entry> LoadAll();

	// The favourites, kept beside the recents. A favourite sorts to the top of the manager's list.
	void SetFavorite(const std::filesystem::path& folder, bool favorite);

	// Takes a project off the recents (and its favourite mark with it). The files stay: the list is the
	// manager's memory, not the disk.
	void RemoveRecent(const std::filesystem::path& folder);

	// Sweeps the recents of every entry whose folder no longer exists.
	void RemoveMissing();

	// Renames the project's marker — the name the manager and the editor show. The folder keeps its name:
	// a rename that moved directories would break every path that reaches the project.
	bool Rename(const std::filesystem::path& folder, const std::string& newName, std::string& error);

	// Copies the project to a sibling folder and puts the copy on the recents. Returns the copy's folder,
	// or an empty path with 'error' saying why not.
	std::filesystem::path Duplicate(const std::filesystem::path& folder, std::string& error);

	// Teaches Windows that a .lnproject opens this editor — and takes back the .lscene claim older versions
	// made: a scene opens inside the editor, the way it does in Unreal and Visual Studio.
	void RegisterFileType();
}
