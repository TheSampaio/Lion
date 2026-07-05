#pragma once

namespace Lion
{
	// Resolves an asset path for the current run.
	//
	// Returns the path unchanged when the file exists relative to the working directory
	// (a distributed build runs next to its flattened resources), otherwise falls back to a
	// "Resource/"-prefixed path (development runs from the project folder, where assets live
	// under Resource/). If neither exists, the original path is returned for error reporting.
	std::string ResolveResourcePath(const std::string& path);
}
