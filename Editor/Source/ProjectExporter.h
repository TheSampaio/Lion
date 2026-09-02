#pragma once

#include <filesystem>
#include <string>

namespace ProjectExporter
{
	struct Options
	{
		std::filesystem::path destination;
		std::string executableName;
		bool sealAssets = true;
		bool includeLicenses = true;
		bool includeIcons = true;
	};

	struct Result
	{
		bool succeeded = false;
		std::filesystem::path outputDirectory;
		std::string message;
		std::string buildOutput;
	};

	// Builds the active game's module in Shipping and packages the Windows player runtime, assets and
	// licences into a new folder under destination. Existing exports are never overwritten.
	Result ExportWindows(const std::filesystem::path& project, const Options& options);
	Result ExportWindows(const std::filesystem::path& project, const std::filesystem::path& destination);
}
