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
	constexpr const char8* kExtensionFlag = "--seal";
	constexpr const char8* kAssetsFlag = "--seal-assets";

	bool IsPackagedAsset(const std::filesystem::path& path)
	{
		const std::filesystem::path extension = path.extension();
		return extension != ".cpp" && extension != ".h" && extension != ".hpp" && extension != ".lnexport";
	}

	// Sealing something already sealed would ruin it, so each file is asked which it is. The build copies
	// fresh plaintext assets before this runs, but a step that is only safe when nobody runs it twice is not
	// a safe step.
	bool SealFile(const std::filesystem::path& file, bool& changed, std::string& error)
	{
		std::ifstream input(file, std::ios::binary);

		if (!input.is_open())
		{
			error = "Could not open '" + file.generic_string() + "' for sealing.";
			return false;
		}

		std::stringstream buffer;
		buffer << input.rdbuf();
		input.close();

		const std::string content = buffer.str();

		if (Vault::IsSealed(content))
		{
			changed = false;
			return true;
		}

		std::ofstream output(file, std::ios::binary | std::ios::trunc);

		if (!output.is_open())
		{
			error = "Could not open '" + file.generic_string() + "' for sealing.";
			return false;
		}

		output << Vault::Seal(content);
		changed = output.good();

		if (!changed)
			error = "Could not seal '" + file.generic_string() + "'.";

		return changed;
	}
}

bool Sealer::SealAssets(const std::filesystem::path& sourceAssets,
	const std::filesystem::path& packagedDirectory, std::string& error, int32* sealedCount)
{
	std::error_code code;
	int32 sealed = 0;

	for (std::filesystem::recursive_directory_iterator it(sourceAssets, code), end; it != end; it.increment(code))
	{
		if (code)
			break;

		if (it->is_directory(code) && it->path().filename() == "Scripts")
		{
			it.disable_recursion_pending();
			continue;
		}

		if (!it->is_regular_file(code) || !IsPackagedAsset(it->path()))
			continue;

		const std::filesystem::path target = packagedDirectory / it->path().lexically_relative(sourceAssets);
		bool changed = false;

		if (!SealFile(target, changed, error))
			return false;

		if (changed)
			sealed++;
	}

	if (code)
	{
		error = "Could not traverse '" + sourceAssets.generic_string() + "': " + code.message();
		return false;
	}

	if (sealedCount)
		*sealedCount = sealed;

	return true;
}

std::optional<int32> Sealer::RunFromCommandLine()
{
	if (CommandLine::GetCount() < 2)
		return std::nullopt;

	if (CommandLine::Get(1) == kAssetsFlag)
	{
		if (CommandLine::GetCount() != 4)
		{
			std::cerr << "usage: Lion.exe " << kAssetsFlag << " <source-assets> <packaged-directory>\n";
			return EXIT_FAILURE;
		}

		std::string error;
		int32 sealed = 0;

		if (!SealAssets(CommandLine::Get(2), CommandLine::Get(3), error, &sealed))
		{
			std::cerr << error << '\n';
			return EXIT_FAILURE;
		}

		std::cout << "[Seal] Sealed " << sealed << " packaged asset(s).\n";
		return EXIT_SUCCESS;
	}

	if (CommandLine::Get(1) != kExtensionFlag)
		return std::nullopt;

	if (CommandLine::GetCount() < 4)
	{
		std::cerr << "usage: Lion.exe " << kExtensionFlag << " <directory> <extension> [<extension> ...]\n";
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

		bool changed = false;
		std::string sealError;

		if (!SealFile(entry.path(), changed, sealError))
		{
			std::cerr << sealError << '\n';
			return EXIT_FAILURE;
		}

		if (changed)
			sealed++;
	}

	std::cout << "[Seal] Sealed " << sealed << " file(s) under " << directory.string() << ".\n";
	return EXIT_SUCCESS;
}
