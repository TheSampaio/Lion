#include "Editor.h"

Lion::Application* Lion::Main()
{
	// The editor is a tool, and a tool that cannot tell you what happened is not one — so it keeps
	// every diagnostic the engine has, whatever configuration it was built in. The build's own levels
	// are meant for a game that ships, not for the thing you debug with.
	//
	// Set before the application is constructed, so even the engine's start-up messages are kept.
	Log::SetVerbosity(LogVerbosity::Verbose);
	Clock::SetShowFrameStats(true);

	return new Editor();
}
