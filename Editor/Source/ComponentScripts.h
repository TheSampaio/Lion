#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Language-specific source generation for game components.
//
// Scenes and the Inspector know components only by their registered name and reflected fields. Keeping
// source templates behind this boundary preserves that language-neutral model when another backend,
// such as C#, is introduced.
namespace ComponentScripts
{
	enum class Language
	{
		Cpp,
	};

	struct LanguageInfo
	{
		Language language;
		const char* displayName;
		const char* fileSuffix;
	};

	// Returns the source languages the editor can scaffold, in picker order.
	const std::vector<LanguageInfo>& Languages();

	// Generates a component's source files in `directory`.
	//
	// Args:
	//     language: Source backend selected in the editor.
	//     name: Valid component type name.
	//     directory: Existing or creatable project directory.
	//     error: Receives a user-facing failure reason.
	//
	// Returns:
	//     True when every source file was written.
	bool Generate(Language language, const std::string& name,
		const std::filesystem::path& directory, std::string& error);
}
