#include "Editor.h"

#include <cstdlib>

#include "Source/Sealer.h"

Lion::Application* Lion::Main()
{
	// The build runs the editor to seal a shipped game's assets, and for nothing else — so it seals them and
	// the process is over, before a window is opened or a scene is loaded.
	if (const std::optional<int32> code = Sealer::RunFromCommandLine())
		std::exit(*code);

	// The editor is a tool, and a tool that cannot tell you what happened is not one — so it keeps
	// every diagnostic the engine has, whatever configuration it was built in. The build's own levels
	// are meant for a game that ships, not for the thing you debug with.
	//
	// Set before the application is constructed, so even the engine's start-up messages are kept.
	Log::SetVerbosity(LogVerbosity::Verbose);

	// The title bar stops keeping score. The editor has a Statistics panel that says all of it and more,
	// and a title that rewrites itself every second is a title nobody can read. A game still keeps them —
	// it has nowhere else to put them — except when it ships, where nothing is kept.
	Clock::SetShowFrameStats(false);

	return new Editor();
}
