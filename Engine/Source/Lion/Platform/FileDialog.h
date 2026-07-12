#pragma once

namespace Lion
{
	// Native open/save file dialogs.
	//
	// The filter uses the Win32 double-null format, e.g. "Lion Scene (*.json)\0*.json\0".
	// Both return the chosen path, or an empty string when the dialog is cancelled.
	// 'initialDirectory' is where the dialog opens: an asset belongs to the project, so browsing for one
	// starts inside it rather than wherever the user happened to be last.
	class FileDialog
	{
	public:
		static LION_API std::string Open(const char8* filter, const std::string& initialDirectory = std::string());
		static LION_API std::string Save(const char8* filter, const char8* defaultExtension, const std::string& initialDirectory = std::string());
	};
}
