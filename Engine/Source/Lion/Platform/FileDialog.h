#pragma once

namespace Lion
{
	// Native open/save file dialogs.
	//
	// The filter uses the Win32 double-null format, e.g. "Lion Scene (*.json)\0*.json\0".
	// Both return the chosen path, or an empty string when the dialog is cancelled.
	class FileDialog
	{
	public:
		static LION_API std::string Open(const char8* filter);
		static LION_API std::string Save(const char8* filter, const char8* defaultExtension);
	};
}
