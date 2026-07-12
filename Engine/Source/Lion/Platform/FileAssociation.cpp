#include "Engine.h"
#include "FileAssociation.h"

#include <Lion/Core/Log.h>

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <ShlObj.h>
#endif

namespace Lion
{
#ifdef LN_PLATFORM_WIN
	namespace
	{
		bool WriteKey(const std::string& path, const std::string& value)
		{
			HKEY key = nullptr;

			if (RegCreateKeyExA(HKEY_CURRENT_USER, path.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
				KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
				return false;

			const LSTATUS status = RegSetValueExA(key, nullptr, 0, REG_SZ,
				reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>(value.size() + 1));

			RegCloseKey(key);
			return status == ERROR_SUCCESS;
		}

		std::string ReadKey(const std::string& path)
		{
			char8 buffer[MAX_PATH * 2] = {};
			DWORD size = sizeof(buffer);

			if (RegGetValueA(HKEY_CURRENT_USER, path.c_str(), nullptr, RRF_RT_REG_SZ, nullptr, buffer, &size) != ERROR_SUCCESS)
				return {};

			return std::string(buffer);
		}
	}

	bool FileAssociation::Register(const std::string& extension, const std::string& programId,
		const std::string& description, const std::string& iconPath, const std::string& applicationPath)
	{
		const std::string classes = "Software\\Classes\\";
		const std::string command = "\"" + applicationPath + "\" \"%1\"";

		// The one value that changes when the editor moves. Finding it already written is finding all of
		// them already written, so it is the only one worth reading.
		if (ReadKey(classes + programId + "\\shell\\open\\command") == command)
			return false;

		const bool written =
			WriteKey(classes + extension, programId) &&
			WriteKey(classes + programId, description) &&
			WriteKey(classes + programId + "\\DefaultIcon", iconPath) &&
			WriteKey(classes + programId + "\\shell\\open\\command", command);

		if (!written)
		{
			Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[FileAssociation] Could not register '{}'.", extension));
			return false;
		}

		// Explorer caches what it knows about an extension; without this it goes on drawing the blank page
		// it drew before, sometimes until the machine is restarted.
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[FileAssociation] '{}' now opens with this editor.", extension));
		return true;
	}
#else
	bool FileAssociation::Register(const std::string&, const std::string&, const std::string&,
		const std::string&, const std::string&)
	{
		return false;
	}
#endif
}
