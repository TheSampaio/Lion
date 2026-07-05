#include "Engine.h"
#include "Filesystem.h"

namespace Lion
{
	static bool FileExists(const std::string& path)
	{
		return std::ifstream(path).good();
	}

	std::string ResolveResourcePath(const std::string& path)
	{
		if (FileExists(path))
			return path;

		// Development fallback: running from the project folder, assets live under "Resource/".
		const std::string fallback = "Resource/" + path;

		if (FileExists(fallback))
			return fallback;

		return path;
	}
}
