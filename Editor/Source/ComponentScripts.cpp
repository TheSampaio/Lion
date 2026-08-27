#include "EditorPch.h"
#include "ComponentScripts.h"

namespace ComponentScripts
{
	namespace
	{
		bool GenerateCpp(const std::string& name, const std::filesystem::path& directory, std::string& error)
		{
			const std::filesystem::path headerPath = directory / (name + ".h");
			const std::filesystem::path sourcePath = directory / (name + ".cpp");

			std::error_code code;

			if (std::filesystem::exists(headerPath, code) || std::filesystem::exists(sourcePath, code))
			{
				error = "A component named '" + name + "' already exists.";
				return false;
			}

			std::ofstream header(headerPath);
			std::ofstream source(sourcePath);

			if (!header.is_open() || !source.is_open())
			{
				header.close();
				source.close();
				std::filesystem::remove(headerPath, code);
				std::filesystem::remove(sourcePath, code);
				error = "Could not write to '" + directory.generic_string() + "'.";
				return false;
			}

			header
				<< "#pragma once\n\n"
				<< "#include <Lion/Lion.h>\n\n"
				<< "// Component defined by the game. Loading its compiled module registers it with the editor.\n"
				<< "class " << name << " : public Lion::Component\n"
				<< "{\n"
				<< "public:\n"
				<< "\t// Initializes the component after its owner enters the scene.\n"
				<< "\tvoid OnAwake() override;\n\n"
				<< "\t// Advances the component once per active frame.\n"
				<< "\tvoid OnUpdate() override;\n\n"
				<< "\t// Describes fields shared by the Inspector and scene serialization.\n"
				<< "\tvoid Reflect(Lion::Reflector& reflector) override;\n\n"
				<< "private:\n"
				<< "\tLion::float32 mSpeed = 1.0f;\n"
				<< "};\n";

			source
				<< "#include \"" << name << ".h\"\n\n"
				<< "#include <Lion/Logic/ComponentRegistry.h>\n"
				<< "#include <Lion/Logic/Reflector.h>\n\n"
				<< "using namespace Lion;\n\n"
				<< "void " << name << "::OnAwake()\n"
				<< "{\n"
				<< "}\n\n"
				<< "void " << name << "::OnUpdate()\n"
				<< "{\n"
				<< "}\n\n"
				<< "void " << name << "::Reflect(Reflector& reflector)\n"
				<< "{\n"
				<< "\treflector.Field(\"Speed\", mSpeed);\n"
				<< "}\n\n"
				<< "LION_REGISTER_COMPONENT(" << name << ")\n";

			return true;
		}
	}

	const std::vector<LanguageInfo>& Languages()
	{
		static const std::vector<LanguageInfo> languages = {
			{ Language::Cpp, "C++", ".h/.cpp" },
		};

		return languages;
	}

	bool Generate(Language language, const std::string& name,
		const std::filesystem::path& directory, std::string& error)
	{
		std::error_code code;
		std::filesystem::create_directories(directory, code);

		if (code)
		{
			error = code.message();
			return false;
		}

		switch (language)
		{
			case Language::Cpp:
				return GenerateCpp(name, directory, error);
		}

		error = "The selected component language is not available.";
		return false;
	}
}
