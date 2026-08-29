#pragma once

#include <filesystem>
#include <string>

namespace ProjectExporter
{
	struct Result
	{
		bool succeeded = false;
		std::filesystem::path outputDirectory;
		std::string message;
		std::string buildOutput;
	};

	// Builds the active game's module in Shipping and packages the Windows player runtime, assets and
	// licences into a new folder under destination. Existing exports are never overwritten.
	Result ExportWindows(const std::filesystem::path& project, const std::filesystem::path& destination);
}
