#include "Sandbox.h"

// Exported entry point of the game module.
//
// The game is built as a shared library (Game.dll) rather than an executable: the standalone
// launcher and the editor both load it at runtime, so the game's code — including any user-defined
// components — is shared by both. Loading the DLL runs its static initializers, which register those
// components; this factory hands back the Application the standalone launcher then runs.
//
// extern "C" keeps the symbol name unmangled so the launcher can resolve it by name.
extern "C" __declspec(dllexport) Lion::Application* LionCreateApplication()
{
	return new Sandbox();
}
