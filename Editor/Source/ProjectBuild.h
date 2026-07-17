#pragma once

#include <filesystem>
#include <string>

#include <Lion/Lion.h>

// A project's own Visual Studio build, tied to the engine the way Unreal ties a game to an installed
// engine: the editor generates <project>/Build/lion-game.vcxproj against the SDK beside itself — the
// engine's headers and import library — plus a solution at the project's root for working in Visual
// Studio directly. The editor regenerates the .vcxproj before every compile, because the file list is a
// glob of the project and a file just scaffolded is not in the old one.
//
// This is what lets a *distributed* editor create and compile C++ components: it needs Visual Studio on
// the machine (found through vswhere, as Unreal needs it), and nothing of the engine's source tree.
namespace ProjectBuild
{
	// The SDK beside the editor, or nothing when there is none to compile against.
	std::filesystem::path SdkDirectory();

	// Where the project's compiled module lands — and where the editor looks for a project's own module
	// before falling back to the one beside itself.
	std::filesystem::path ModulePath(const std::filesystem::path& project);

	// The project file the editor builds. Under Build/, out of the way of the assets.
	std::filesystem::path VcxprojPath(const std::filesystem::path& project);

	// Writes the .vcxproj (fresh file list) and the .sln (once; it never changes). Returns whether it
	// wrote, with 'error' saying why not.
	bool Generate(const std::filesystem::path& project, std::string& error);
}
