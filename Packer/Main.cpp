#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Not the umbrella header: it reaches Application, which reaches the entry point the engine writes for a
// game. A tool writes its own.
#include <Lion/Base/Platform.h>
#include <Lion/Base/Standard.h>
#include <Lion/Type/Primitive.h>
#include <Lion/Core/Vault.h>

// Seals a shipped game's assets in place.
//
// The build used to do this in PowerShell, which meant the format lived twice — once where a file is
// written and once where it is read — and the two would have drifted the first time either of them
// changed. It lives once now, in the engine, and this is the engine asking it to.
//
//     lion-seal <directory> <extension> [<extension> ...]
//
// Sealing something already sealed would ruin it, so the tool checks. The build copies fresh plaintext
// assets before this runs, but a post-build step that is only safe when nobody runs it twice is not safe.
int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::cerr << "usage: lion-seal <directory> <extension> [<extension> ...]\n";
		return 1;
	}

	const std::filesystem::path directory = argv[1];

	std::vector<std::string> extensions;
	for (int argument = 2; argument < argc; ++argument)
		extensions.emplace_back(argv[argument]);

	std::error_code error;

	if (!std::filesystem::is_directory(directory, error))
		return 0;   // Nothing shipped there; nothing to seal.

	int sealed = 0;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, error))
	{
		if (!entry.is_regular_file())
			continue;

		const std::string extension = entry.path().extension().string();

		if (std::find(extensions.begin(), extensions.end(), extension) == extensions.end())
			continue;

		std::ifstream input(entry.path(), std::ios::binary);
		std::stringstream buffer;
		buffer << input.rdbuf();
		input.close();

		const std::string content = buffer.str();

		if (Lion::Vault::IsSealed(content))
			continue;

		std::ofstream output(entry.path(), std::ios::binary | std::ios::trunc);
		output << Lion::Vault::Seal(content);

		sealed++;
	}

	std::cout << "[Seal] Sealed " << sealed << " file(s) under " << directory.string() << ".\n";
	return 0;
}
