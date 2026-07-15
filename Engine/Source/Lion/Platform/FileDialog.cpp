#include "Engine.h"
#include "FileDialog.h"

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <commdlg.h>
	#include <ShObjIdl.h>
	#pragma comment(lib, "Comdlg32.lib")
#endif

namespace Lion
{
#ifdef LN_PLATFORM_WIN
	std::string FileDialog::Open(const char8* filter, const std::string& initialDirectory)
	{
		CHAR file[MAX_PATH] = { 0 };

		OPENFILENAMEA openFileName = { };
		openFileName.lStructSize = sizeof(OPENFILENAMEA);
		openFileName.hwndOwner = GetActiveWindow();
		openFileName.lpstrFile = file;
		openFileName.nMaxFile = sizeof(file);
		openFileName.lpstrFilter = filter;
		openFileName.nFilterIndex = 1;
		openFileName.lpstrInitialDir = initialDirectory.empty() ? nullptr : initialDirectory.c_str();
		openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&openFileName) == TRUE)
			return openFileName.lpstrFile;

		return std::string();
	}

	std::string FileDialog::OpenFolder(const std::string& initialDirectory)
	{
		// The modern shell picker (IFileDialog with FOS_PICKFOLDERS), not the old SHBrowseForFolder tree —
		// it is the dialog every current application shows, and it navigates like the file one beside it.
		std::string result;

		if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
			return result;

		IFileDialog* dialog = nullptr;

		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
		{
			DWORD options = 0;
			dialog->GetOptions(&options);
			dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

			// Where it opens: the folder handed in, if it is one. A path is UTF-8 here and UTF-16 there.
			if (!initialDirectory.empty())
			{
				const int wide = MultiByteToWideChar(CP_UTF8, 0, initialDirectory.c_str(), -1, nullptr, 0);
				std::wstring wideDir(wide > 0 ? wide - 1 : 0, L' ');   // -1: the returned count includes the terminator.
				MultiByteToWideChar(CP_UTF8, 0, initialDirectory.c_str(), -1, wideDir.data(), wide);

				IShellItem* folder = nullptr;
				if (SUCCEEDED(SHCreateItemFromParsingName(wideDir.c_str(), nullptr, IID_PPV_ARGS(&folder))))
				{
					dialog->SetFolder(folder);
					folder->Release();
				}
			}

			if (SUCCEEDED(dialog->Show(GetActiveWindow())))
			{
				IShellItem* item = nullptr;

				if (SUCCEEDED(dialog->GetResult(&item)))
				{
					PWSTR path = nullptr;

					if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
					{
						const int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
						result.resize(size > 0 ? size - 1 : 0);
						WideCharToMultiByte(CP_UTF8, 0, path, -1, result.data(), size, nullptr, nullptr);
						CoTaskMemFree(path);
					}

					item->Release();
				}
			}

			dialog->Release();
		}

		CoUninitialize();
		return result;
	}

	std::string FileDialog::Save(const char8* filter, const char8* defaultExtension, const std::string& initialDirectory)
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
		openFileName.lpstrInitialDir = initialDirectory.empty() ? nullptr : initialDirectory.c_str();
		openFileName.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&openFileName) == TRUE)
			return openFileName.lpstrFile;

		return std::string();
	}
#else
	std::string FileDialog::Open(const char8*, const std::string&) { return std::string(); }
	std::string FileDialog::OpenFolder(const std::string&) { return std::string(); }
	std::string FileDialog::Save(const char8*, const char8*, const std::string&) { return std::string(); }
#endif
}
