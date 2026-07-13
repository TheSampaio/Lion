#pragma once

#include <optional>

#include <Lion/Type/Primitive.h>

// Sealing a shipped game's assets, which the build asks the editor to do on its way past.
//
// It was a project of its own, and it did not need to be one: it is a directory walk around a format that
// already lives in the engine (Lion/Core/Vault.h). A whole executable — with its own premake file, its own
// copy of the engine beside it and its own place in the solution — to spell a loop that runs once per ship.
//
// The editor is where it belongs, because building a game is a thing an editor does. It runs before there is
// a window, an application or a scene, and then the process is over:
//
//     Lion.exe --seal <directory> <extension> [<extension> ...]
class Sealer
{
public:
	// The exit code the editor should end on, or nothing at all when it was started to be an editor.
	static std::optional<Lion::int32> RunFromCommandLine();
};
