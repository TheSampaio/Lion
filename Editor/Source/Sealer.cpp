#include "EditorPch.h"
#include "Sealer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// The engine's headers export with LION_API and speak in its own types; these are where those come from.
// Not the umbrella header, which reaches the entry point — this file runs before the application exists.
#include <Lion/Base/Platform.h>
#include <Lion/Base/Standard.h>
#include <Lion/Core/CommandLine.h>
#include <Lion/Core/Vault.h>

using namespace Lion;

namespace
{
	constexpr const char8* kFlag = "--seal";

	// Sealing something already sealed would ruin it, so each file is asked which it is. The build copies
	// fresh plaintext assets before this runs, but a step that is only safe when nobody runs it twice is not
	// a safe step.
	bool SealFile(const std::filesystem::path& file)
	{
		std::ifstream input(file, std::ios::binary);
		std::stringstream buffer;
		buffer << input.rdbuf();
		input.close();

		const std::string content = buffer.str();

		if (Vault::IsSealed(content))
			return false;

		std::ofstream output(file, std::ios::binary | std::ios::trunc);
		output << Vault::Seal(content);
		return true;
	}
}

std::optional<int32> Sealer::RunFromCommandLine()
{
	if (CommandLine::GetCount() < 2 || CommandLine::Get(1) != kFlag)
		return std::nullopt;

	if (CommandLine::GetCount() < 4)
	{
		std::cerr << "usage: Lion.exe " << kFlag << " <directory> <extension> [<extension> ...]\n";
		return EXIT_FAILURE;
	}

	const std::filesystem::path directory = CommandLine::Get(2);

	std::vector<std::string> extensions;
	for (int32 argument = 3; argument < CommandLine::GetCount(); ++argument)
		extensions.emplace_back(CommandLine::Get(argument));

	std::error_code error;

	if (!std::filesystem::is_directory(directory, error))
		return EXIT_SUCCESS;   // Nothing shipped there; nothing to seal.

	int32 sealed = 0;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, error))
	{
		if (!entry.is_regular_file())
			continue;

		const std::string extension = entry.path().extension().string();

		if (std::find(extensions.begin(), extensions.end(), extension) == extensions.end())
			continue;

		if (SealFile(entry.path()))
			sealed++;
	}

	std::cout << "[Seal] Sealed " << sealed << " file(s) under " << directory.string() << ".\n";
	return EXIT_SUCCESS;
}
