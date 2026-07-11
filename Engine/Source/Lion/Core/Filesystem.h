#pragma once

namespace Lion
{
	// Resolves an asset path for the current run.
	//
	// Returns the path unchanged when the file exists relative to the working directory
	// (a distributed build runs next to its flattened assets), otherwise falls back to an
	// "Assets/"-prefixed path (development runs from the project folder, where they live under
	// Assets/). If neither exists, the original path is returned for error reporting.
	std::string ResolveResourcePath(const std::string& path);

	// Converts an absolute asset path into one relative to the resource root (the executable
	// directory), using forward slashes — suitable for storing in a scene. Returns the input
	// unchanged when it is not located under the resource root.
	LION_API std::string ToResourceRelativePath(const std::string& absolutePath);

	// Root the editor browses for assets: the directory the build copies resources into, i.e. the
	// executable's directory (with a trailing separator). Empty when it cannot be determined.
	LION_API const std::string& ResourceRootDirectory();
}
