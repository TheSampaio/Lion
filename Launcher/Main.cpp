#include <Lion/Lion.h>
#include <Lion/Launcher.h>

#include <Lion/Core/DynamicLibrary.h>
#include <Lion/Core/Log.h>

// Standalone game launcher.
//
// The launcher owns no game code. It loads the game module (Game.dll) — which registers the game's
// components as a side effect of loading — and runs the Application the module hands back, so the
// exact same DLL is shared by the standalone game and the editor. The module handle is kept alive
// for the whole run so the game's code stays mapped until the application is destroyed.
Lion::Application* Lion::Main()
{
	static Lion::DynamicLibrary gameModule;

	if (!gameModule.Load("Game.dll"))
	{
		Lion::Log::Console(Lion::LogLevel::Fatal, "[Launcher] Could not load the game module 'Game.dll'.");
		return nullptr;
	}

	using CreateApplication = Lion::Application* (*)();
	const auto create = gameModule.GetFunction<CreateApplication>("LionCreateApplication");

	if (!create)
	{
		Lion::Log::Console(Lion::LogLevel::Fatal, "[Launcher] 'Game.dll' does not export LionCreateApplication.");
		return nullptr;
	}

	return create();
}
