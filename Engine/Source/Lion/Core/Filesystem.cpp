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

	static std::string& ResourceOverrideDirectory()
	{
		static std::string directory;
		return directory;
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
		// An absolute path already says exactly where it belongs. Relative paths deliberately wait until
		// after the authored override so a stale packaged copy cannot shadow the active project.
		if (std::filesystem::path(path).is_absolute() && FileExists(path))
			return path;

		// Tools author against the active project's Assets folder, not the last copy produced by a build.
		// This must precede the working directory: Lion's Mane runs beside a packaged asset copy that may be
		// older than the Assembly or Scene currently being edited.
		const std::string& overrideDirectory = ResourceOverrideDirectory();

		if (!overrideDirectory.empty())
		{
			const std::filesystem::path authored = std::filesystem::path(overrideDirectory) / path;

			if (FileExists(authored.string()))
				return authored.string();
		}

		// A standalone run launched from its output folder finds the flattened resource beside itself.
		if (FileExists(path))
			return path;

		// Development fallback: running from the project folder, assets live under "Assets/".
		const std::string devPath = "Assets/" + path;

		if (FileExists(devPath))
			return devPath;

		// Packaged resources live next to the executable, independent of the working directory.
		const std::string& executableDirectory = ExecutableDirectory();

		if (!executableDirectory.empty())
		{
			const std::string exePath = executableDirectory + path;

			if (FileExists(exePath))
				return exePath;
		}

		return path;
	}

	void SetResourceOverrideDirectory(const std::string& directory)
	{
		ResourceOverrideDirectory() = directory.empty()
			? std::string()
			: std::filesystem::absolute(directory).lexically_normal().string();
	}

	const std::string& ResourceRootDirectory()
	{
		return ExecutableDirectory();
	}

	std::string ToResourceRelativePath(const std::string& absolutePath)
	{
		const auto relativeTo = [&](const std::string& root) -> std::string
		{
			if (root.empty())
				return {};

			std::error_code error;
			const std::filesystem::path relative = std::filesystem::relative(absolutePath, root, error);
			const std::string result = error ? std::string() : relative.generic_string();

			return !result.empty() && result.rfind("..", 0) != 0 ? result : std::string();
		};

		if (const std::string authored = relativeTo(ResourceOverrideDirectory()); !authored.empty())
			return authored;

		if (const std::string packaged = relativeTo(ExecutableDirectory()); !packaged.empty())
			return packaged;

		return absolutePath;
	}
}
