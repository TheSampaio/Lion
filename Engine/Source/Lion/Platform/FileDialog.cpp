#include "Engine.h"
#include "FileDialog.h"

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <ShObjIdl.h>
#endif

namespace Lion
{
#ifdef LN_PLATFORM_WIN
	namespace
	{
		// UTF-8 in, UTF-16 out: the engine speaks UTF-8 and the shell speaks wide.
		std::wstring Widen(const std::string& text)
		{
			if (text.empty())
				return {};

			const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
			std::wstring wide(size > 0 ? size - 1 : 0, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
			return wide;
		}

		std::string Narrow(PCWSTR text)
		{
			const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
			std::string narrow(size > 0 ? size - 1 : 0, '\0');
			WideCharToMultiByte(CP_UTF8, 0, text, -1, narrow.data(), size, nullptr, nullptr);
			return narrow;
		}

		// Points the dialog at a folder, and pins it there. SetFolder alone loses to the shell's memory of
		// where this application last browsed — the reason lpstrInitialDir was ignored the second time a
		// sprite was picked. SetDefaultFolder is what overrides that memory, so the dialog opens where it is
		// told every time, not only the first.
		void PointAt(IFileDialog* dialog, const std::string& directory)
		{
			if (directory.empty())
				return;

			IShellItem* folder = nullptr;

			if (SUCCEEDED(SHCreateItemFromParsingName(Widen(directory).c_str(), nullptr, IID_PPV_ARGS(&folder))))
			{
				dialog->SetDefaultFolder(folder);
				dialog->SetFolder(folder);
				folder->Release();
			}
		}

		// The "Label\0pattern\0...\0\0" filter the engine passes, as the COMDLG_FILTERSPEC pairs IFileDialog
		// wants. The wide strings have to outlive SetFileTypes, so they are kept in 'storage'.
		std::vector<COMDLG_FILTERSPEC> BuildFilters(const char8* filter, std::vector<std::wstring>& storage)
		{
			std::vector<COMDLG_FILTERSPEC> specs;

			if (!filter)
				return specs;

			// Walk the double-null-terminated list, label then pattern, until an empty label ends it.
			for (const char8* cursor = filter; *cursor;)
			{
				const std::string label = cursor;
				cursor += label.size() + 1;

				if (!*cursor)
					break;

				const std::string pattern = cursor;
				cursor += pattern.size() + 1;

				storage.push_back(Widen(label));
				storage.push_back(Widen(pattern));
			}

			for (size_t i = 0; i + 1 < storage.size(); i += 2)
				specs.push_back({ storage[i].c_str(), storage[i + 1].c_str() });

			return specs;
		}

		// Runs a configured dialog and returns the chosen item's filesystem path, or empty when cancelled.
		std::string Show(IFileDialog* dialog)
		{
			std::string result;

			if (FAILED(dialog->Show(GetActiveWindow())))
				return result;

			IShellItem* item = nullptr;

			if (SUCCEEDED(dialog->GetResult(&item)))
			{
				PWSTR path = nullptr;

				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
				{
					result = Narrow(path);
					CoTaskMemFree(path);
				}

				item->Release();
			}

			return result;
		}

		// One entry point for every kind of dialog, since the only differences are the CLSID, the options and
		// whether a default extension is set. COM is initialised per call: the editor draws on one thread, so
		// there is no shared apartment to keep alive between them.
		std::string RunDialog(REFCLSID which, FILEOPENDIALOGOPTIONS options, const char8* filter,
			const char8* defaultExtension, const std::string& directory)
		{
			if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
				return {};

			std::string result;
			IFileDialog* dialog = nullptr;

			if (SUCCEEDED(CoCreateInstance(which, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
			{
				FILEOPENDIALOGOPTIONS current = 0;
				dialog->GetOptions(&current);
				dialog->SetOptions(current | options | FOS_FORCEFILESYSTEM);

				std::vector<std::wstring> storage;
				const std::vector<COMDLG_FILTERSPEC> specs = BuildFilters(filter, storage);

				if (!specs.empty())
					dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());

				if (defaultExtension && *defaultExtension)
					dialog->SetDefaultExtension(Widen(defaultExtension).c_str());

				PointAt(dialog, directory);

				result = Show(dialog);
				dialog->Release();
			}

			CoUninitialize();
			return result;
		}
	}

	std::string FileDialog::Open(const char8* filter, const std::string& initialDirectory)
	{
		return RunDialog(CLSID_FileOpenDialog, FOS_FILEMUSTEXIST, filter, nullptr, initialDirectory);
	}

	std::string FileDialog::OpenFolder(const std::string& initialDirectory)
	{
		return RunDialog(CLSID_FileOpenDialog, FOS_PICKFOLDERS, nullptr, nullptr, initialDirectory);
	}

	std::string FileDialog::Save(const char8* filter, const char8* defaultExtension, const std::string& initialDirectory)
	{
		return RunDialog(CLSID_FileSaveDialog, FOS_OVERWRITEPROMPT, filter, defaultExtension, initialDirectory);
	}
#else
	std::string FileDialog::Open(const char8*, const std::string&) { return std::string(); }
	std::string FileDialog::OpenFolder(const std::string&) { return std::string(); }
	std::string FileDialog::Save(const char8*, const char8*, const std::string&) { return std::string(); }
#endif
}
