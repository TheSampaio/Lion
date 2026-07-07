#include "Engine.h"
#include "FileDialog.h"

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <commdlg.h>
	#pragma comment(lib, "Comdlg32.lib")
#endif

namespace Lion
{
#ifdef LN_PLATFORM_WIN
	std::string FileDialog::Open(const char8* filter)
	{
		CHAR file[MAX_PATH] = { 0 };

		OPENFILENAMEA openFileName = { };
		openFileName.lStructSize = sizeof(OPENFILENAMEA);
		openFileName.hwndOwner = GetActiveWindow();
		openFileName.lpstrFile = file;
		openFileName.nMaxFile = sizeof(file);
		openFileName.lpstrFilter = filter;
		openFileName.nFilterIndex = 1;
		openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&openFileName) == TRUE)
			return openFileName.lpstrFile;

		return std::string();
	}

	std::string FileDialog::Save(const char8* filter, const char8* defaultExtension)
	{
		CHAR file[MAX_PATH] = { 0 };

		OPENFILENAMEA openFileName = { };
		openFileName.lStructSize = sizeof(OPENFILENAMEA);
		openFileName.hwndOwner = GetActiveWindow();
		openFileName.lpstrFile = file;
		openFileName.nMaxFile = sizeof(file);
		openFileName.lpstrFilter = filter;
		openFileName.nFilterIndex = 1;
		openFileName.lpstrDefExt = defaultExtension;
		openFileName.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&openFileName) == TRUE)
			return openFileName.lpstrFile;

		return std::string();
	}
#else
	std::string FileDialog::Open(const char8*) { return std::string(); }
	std::string FileDialog::Save(const char8*, const char8*) { return std::string(); }
#endif
}
