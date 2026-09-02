#pragma once

namespace Lion
{
	// Resolves an asset path for the current run.
	//
	// Absolute paths stay absolute. Relative paths first use a tool's authored override, then the working
	// directory, an Assets/ development folder and finally the executable directory. If none exists, the
	// original path is returned for error reporting.
	std::string ResolveResourcePath(const std::string& path);

	// Gives tools an authored resource directory to search before packaged files beside the executable.
	// The standalone player never sets an override; Lion's Mane points it at the active project's Assets
	// folder so scene transitions and every component loader read the same source assets the editor shows.
	LION_API void SetResourceOverrideDirectory(const std::string& directory);

	// Converts an absolute asset path into one relative to the active authored resource override, or to
	// the executable directory when no override contains it, using forward slashes. Returns the input
	// unchanged when it is outside both roots.
	LION_API std::string ToResourceRelativePath(const std::string& absolutePath);

	// Root the editor browses for assets: the directory the build copies resources into, i.e. the
	// executable's directory (with a trailing separator). Empty when it cannot be determined.
	LION_API const std::string& ResourceRootDirectory();
}
