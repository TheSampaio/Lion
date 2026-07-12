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
namespace
{
	// The module outlives Main and is dropped when the game exits — but on the way out it takes the
	// registries with it, because their factories are code inside it. Unloading the library and leaving
	// them behind would have them destroyed after it was unmapped, which crashes every game on exit.
	struct GameModule
	{
		Lion::DynamicLibrary library;

		~GameModule() { Lion::UnloadGameModule(library); }
	};
}

Lion::Application* Lion::Main()
{
	static GameModule module;
	Lion::DynamicLibrary& gameModule = module.library;

	if (!Lion::LoadGameModule(gameModule, Lion::kGameModuleFile))
	{
		Lion::Log::Console(Lion::LogLevel::Fatal,
			LION_FORMAT_TEXT("[Launcher] Could not load the game module '{}'.", Lion::kGameModuleFile));
		return nullptr;
	}

	using CreateApplication = Lion::Application* (*)();
	const auto create = gameModule.GetFunction<CreateApplication>(Lion::kGameModuleEntryPoint);

	if (!create)
	{
		Lion::Log::Console(Lion::LogLevel::Fatal,
			LION_FORMAT_TEXT("[Launcher] '{}' does not export {}.", Lion::kGameModuleFile, Lion::kGameModuleEntryPoint));
		return nullptr;
	}

	return create();
}
