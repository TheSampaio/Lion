#include "Engine.h"
#include "Filesystem.h"

#include <filesystem>

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

namespace Lion
{
	static bool FileExists(const std::string& path)
	{
		return std::ifstream(path).good();
	}

	// Directory of the running executable (with a trailing separator), or empty if unavailable.
	// Resources are copied next to the executable by the build, so this anchors them regardless
	// of the current working directory (Visual Studio project dir, output folder, ...).
	static const std::string& ExecutableDirectory()
	{
		static const std::string directory = []
		{
#ifdef LN_PLATFORM_WIN
			char buffer[MAX_PATH] = { 0 };
			const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
			const std::string path(buffer, length);
			const size_t separator = path.find_last_of("\\/");
			return (separator != std::string::npos) ? path.substr(0, separator + 1) : std::string();
#else
			return std::string();
#endif
		}();

		return directory;
	}

	std::string ResolveResourcePath(const std::string& path)
	{
		// 1. Relative to the working directory (standalone run from the output folder).
		if (FileExists(path))
			return path;

		// 2. Development fallback: running from the project folder, assets live under "Resource/".
		const std::string devPath = "Resource/" + path;

		if (FileExists(devPath))
			return devPath;

		// 3. Next to the executable (robust: independent of the working directory).
		const std::string& executableDirectory = ExecutableDirectory();

		if (!executableDirectory.empty())
		{
			const std::string exePath = executableDirectory + path;

			if (FileExists(exePath))
				return exePath;
		}

		return path;
	}

	const std::string& ResourceRootDirectory()
	{
		return ExecutableDirectory();
	}

	std::string ToResourceRelativePath(const std::string& absolutePath)
	{
		const std::string& root = ExecutableDirectory();

		if (!root.empty())
		{
			std::error_code error;
			const std::filesystem::path relative = std::filesystem::relative(absolutePath, root, error);
			const std::string result = error ? std::string() : relative.generic_string();

			// Keep it only when the file actually lives under the resource root (no ".." escapes).
			if (!result.empty() && result.rfind("..", 0) != 0)
				return result;
		}

		return absolutePath;
	}
}
