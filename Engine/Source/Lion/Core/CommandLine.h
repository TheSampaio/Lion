#pragma once

namespace Lion
{
	// What the process was started with.
	//
	// It exists because a file association is only half a feature without it: Windows opens a .lscene by
	// running the editor and handing it the path, and an editor that cannot read its own arguments would
	// take the double-click and show an empty scene.
	//
	// The entry point fills it before the application is built, so a layer can read it from OnCreate.
	class CommandLine
	{
	public:
		static LION_API int32 GetCount();

		// The argument at 'index', or an empty string when there is none. Index 0 is the executable.
		static LION_API const std::string& Get(int32 index);

		// Called by the engine's entry point (see Lion/Launcher.h), and by nothing else.
		static LION_API void Set(int32 count, const char8* const* values);
	};
}
