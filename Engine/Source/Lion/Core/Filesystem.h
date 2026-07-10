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

	// Converts an absolute asset path into one relative to the resource root (the executable
	// directory), using forward slashes — suitable for storing in a scene. Returns the input
	// unchanged when it is not located under the resource root.
	LION_API std::string ToResourceRelativePath(const std::string& absolutePath);
}
