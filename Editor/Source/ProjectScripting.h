#pragma once

#include <filesystem>
#include <string>

// Managed project tooling, not gameplay. Uses the installed SDK without requiring engine sources.
namespace ProjectScripting
{
	bool HasSources(const std::filesystem::path& project);
	std::filesystem::path AssemblyPath(const std::filesystem::path& project, const std::string& configuration);
	bool Build(const std::filesystem::path& project, const std::string& configuration,
		const std::filesystem::path& sdkDirectory, std::string& output, std::string& error);

	// Called on the engine thread before deserializing authored scenes. Loads a private assembly copy
	// and keeps managed lifecycle dormant until Play; no C# source means no .NET dependency.
	bool Load(const std::filesystem::path& project, const std::string& configuration,
		const std::filesystem::path& sdkDirectory, std::string& error);
	void Unload();

	// Packages the SDK, compiled scripts and an app-local .NET runtime from the installed SDK.
	bool PackPlayer(const std::filesystem::path& project, const std::filesystem::path& sdkDirectory,
		const std::filesystem::path& destination, std::string& runtimeVersion, std::string& error);
}
