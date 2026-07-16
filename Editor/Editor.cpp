#include "EditorPch.h"
#include "Editor.h"
#include "ProjectManager.h"

#include <cstdlib>

#include "Source/Sealer.h"

namespace
{
	// Whether the command line already answers the Project Manager's question. A project handed over
	// (--project, what the manager itself passes), the development skip (--no-project-manager), or a scene
	// path (what double-clicking a .lscene passes) all say which game this session is about — anything
	// else, and the manager is there to ask.
	bool WantsEditor()
	{
		using namespace Lion;

		for (int32 argument = 1; argument < CommandLine::GetCount(); ++argument)
		{
			const std::string& value = CommandLine::Get(argument);

			if (value == "--project" || value == "--no-project-manager")
				return true;

			// Leading dashes mark a flag; anything else is a path being opened.
			if (value.rfind("--", 0) != 0)
				return true;
		}

		return false;
	}
}

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

	// The greeting every engine gives: started bare, the executable is the Project Manager, and picking a
	// project starts the editor on it as its own process. Started with a project already named, it is the
	// editor.
	if (!WantsEditor())
		return new ProjectManager();

	return new Editor();
}
