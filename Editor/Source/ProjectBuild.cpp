#include "EditorPch.h"
#include "ProjectBuild.h"

#include "Projects.h"

#include <Lion/Core/Filesystem.h>
#include <cctype>
#include <regex>

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

using namespace Lion;

namespace ProjectBuild
{
	namespace
	{
		// One project, one GUID: the solution and the project file agree by construction, and nothing else
		// ever references it.
		constexpr const char8* kProjectGuid = "{B27D5FA1-8F5C-4E6B-9C7A-2D51E0C4A9F3}";

		// The headers and the import library beside the editor, assembled by Scripts/PackSdk.bat: one
		// merged include tree (every package keeps to its own subfolder, so one path serves them all) and
		// the lib the module links.
		std::filesystem::path DefaultSdkDirectory()
		{
			return ResourceRootDirectory();
		}

		std::filesystem::path IncludeDirectory(const std::filesystem::path& sdkDirectory)
		{
			return sdkDirectory / "Include";
		}

		std::filesystem::path LibraryDirectory(const std::filesystem::path& sdkDirectory)
		{
			return sdkDirectory / "Bin";
		}

		bool IsEmptyScaffold(const std::filesystem::path& source)
		{
			if (source.filename() != "GameModule.cpp") return false;
			std::ifstream file(source);
			std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			static const std::regex comments(R"(//[^\r\n]*|/\*[\s\S]*?\*/)");
			text = std::regex_replace(text, comments, "");
			text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char value) { return std::isspace(value); }), text.end());
			return text == "#include<Lion/Lion.h>extern\"C\"__declspec(dllexport)Lion::Application*LionCreateApplication(){returnnewLion::Application();}";
		}

		// The sources the module is built from: everything the project keeps, except what the build itself
		// writes under Build/ — compiling your own output is how a glob eats its tail.
		void CollectSources(const std::filesystem::path& project,
			std::vector<std::filesystem::path>& compile, std::vector<std::filesystem::path>& include)
		{
			std::error_code error;

			for (std::filesystem::recursive_directory_iterator it(project, error), end; it != end; it.increment(error))
			{
				if (it->is_directory(error))
				{
					if (it->path().filename() == "Build")
						it.disable_recursion_pending();

					continue;
				}

				const std::filesystem::path extension = it->path().extension();

				if (extension == ".cpp")
					compile.push_back(it->path());
				else if (extension == ".h" || extension == ".hpp")
					include.push_back(it->path());
			}
		}
	}

	const std::string& PlatformToolset()
	{
		static const std::string toolset = []
		{
			char8* programFiles = nullptr;
			size_t programFilesLength = 0;

			if (_dupenv_s(&programFiles, &programFilesLength, "ProgramFiles") != 0 || !programFiles)
				return std::string("v143");

			const std::filesystem::path visualStudio =
				std::filesystem::path(programFiles) / "Microsoft Visual Studio";
			std::free(programFiles);
			std::string newest;
			std::error_code error;

			// Installation/version/product paths vary, but the final VC layout does not. Walking only these
			// few directory levels avoids a recursive scan across the entire Visual Studio installation.
			for (const auto& version : std::filesystem::directory_iterator(visualStudio, error))
			{
				for (const auto& product : std::filesystem::directory_iterator(version.path(), error))
				{
					const std::filesystem::path vc = product.path() / "MSBuild" / "Microsoft" / "VC";

					for (const auto& vcVersion : std::filesystem::directory_iterator(vc, error))
					{
						const std::filesystem::path toolsets =
							vcVersion.path() / "Platforms" / "x64" / "PlatformToolsets";

						for (const auto& candidate : std::filesystem::directory_iterator(toolsets, error))
						{
							const std::string name = candidate.path().filename().string();

							if (candidate.is_directory(error) && name > newest)
								newest = name;
						}
					}
				}
			}

			return newest.empty() ? std::string("v143") : newest;
		}();

		return toolset;
	}

	bool Available()
	{
		return Available(DefaultSdkDirectory());
	}

	bool HasNativeSources(const std::filesystem::path& project)
	{
		std::vector<std::filesystem::path> compile, include;
		CollectSources(project, compile, include);
		return std::any_of(compile.begin(), compile.end(), [](const auto& source) { return !IsEmptyScaffold(source); });
	}

	bool Available(const std::filesystem::path& sdkDirectory)
	{
		std::error_code error;
		return std::filesystem::is_directory(IncludeDirectory(sdkDirectory), error)
			&& std::filesystem::exists(LibraryDirectory(sdkDirectory) / "lion-core.lib", error);
	}

	std::filesystem::path ModulePath(const std::filesystem::path& project)
	{
		return ModulePath(project, BuildConfiguration());
	}

	std::filesystem::path ModulePath(const std::filesystem::path& project, const std::string& configuration)
	{
		return project / "Build" / "Bin" / configuration / kGameModuleFile;
	}

	std::filesystem::path VcxprojPath(const std::filesystem::path& project)
	{
		return project / "Build" / "lion-game.vcxproj";
	}

	bool Generate(const std::filesystem::path& project, std::string& error)
	{
		return Generate(project, BuildConfiguration(), DefaultSdkDirectory(), error);
	}

	bool Generate(const std::filesystem::path& project, const std::string& configuration,
		const std::filesystem::path& sdkDirectory, std::string& error)
	{
		if (!Available(sdkDirectory))
		{
			error = "The editor's Include and Bin folders are missing; there is nothing to compile against.";
			return false;
		}

		std::error_code code;
		std::filesystem::create_directories(project / "Build", code);

		if (code)
		{
			error = code.message();
			return false;
		}

		std::vector<std::filesystem::path> compile;
		std::vector<std::filesystem::path> include;
		CollectSources(project, compile, include);

		// The original editor scaffold returned an empty Application. Supply the scene driver in the
		// generated build instead of rewriting authored files or making C# developers edit C++.
		bool customEntry = false;
		compile.erase(std::remove_if(compile.begin(), compile.end(), [&](const auto& source)
		{
			if (IsEmptyScaffold(source)) return true;
			std::ifstream file(source);
			const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			const size_t entry = text.find(kGameModuleEntryPoint);
			if (entry == std::string::npos) return false;
			customEntry = true;
			return false;
		}), compile.end());
		if (!customEntry)
		{
			const auto bootstrap = project / "Build" / "GameModule.cpp";
			std::ofstream file(bootstrap, std::ios::trunc);
			file << "#include <Lion/Lion.h>\n#include <Lion/Runtime/ScenePlayer.h>\n\n"
				<< "extern \"C\" __declspec(dllexport) Lion::Application* LionCreateApplication()\n"
				<< "{\n\treturn Lion::CreateScenePlayer();\n}\n";
			file.close();
			if (!file) { error = "Could not write the scene-player bootstrap."; return false; }
			compile.push_back(bootstrap);
		}

		// The one configuration this editor can load: the module shares the engine's C++ runtime, so it is
		// built the way the engine running it was built — a Debug editor takes a Debug module, an optimised
		// one takes an optimised module, and mixing the two is an allocator dispute inside one process.
		const bool debug = configuration == "Debug";

		const std::string includes = IncludeDirectory(sdkDirectory).generic_string() + ";";

		// LN_DISABLE_WARNINGS mirrors the workspace define in premake5.lua — the engine's headers hand it
		// to #pragma warning, and a build without it trips over the bare macro name.
		const std::string defines = std::string("LN_PLATFORM_WIN;LN_DISABLE_WARNINGS=6294 26495 26498 26800;")
			+ (debug ? "LN_DEBUG;_DEBUG" : (configuration == "Shipping" ? "LN_SHIPPING;NDEBUG" : "LN_RELEASE;NDEBUG"));

		// --- The project file, regenerated whole: it is a build description, not a document anyone edits.
		std::ofstream vcxproj(VcxprojPath(project), std::ios::trunc);

		if (!vcxproj.is_open())
		{
			error = "Could not write " + VcxprojPath(project).generic_string() + ".";
			return false;
		}

		vcxproj
			<< "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			<< "<!-- Generated by the Lion editor before every compile: the file list is a glob of the project. -->\n"
			<< "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
			<< "  <ItemGroup Label=\"ProjectConfigurations\">\n"
			<< "    <ProjectConfiguration Include=\"" << configuration << "|x64\">\n"
			<< "      <Configuration>" << configuration << "</Configuration>\n"
			<< "      <Platform>x64</Platform>\n"
			<< "    </ProjectConfiguration>\n"
			<< "  </ItemGroup>\n"
			<< "  <PropertyGroup Label=\"Globals\">\n"
			<< "    <ProjectGuid>" << kProjectGuid << "</ProjectGuid>\n"
			<< "    <RootNamespace>Game</RootNamespace>\n"
			<< "  </PropertyGroup>\n"
			<< "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n"
			<< "  <PropertyGroup Label=\"Configuration\">\n"
			<< "    <ConfigurationType>DynamicLibrary</ConfigurationType>\n"
			<< "    <PlatformToolset>" << PlatformToolset() << "</PlatformToolset>\n"
			<< "    <CharacterSet>Unicode</CharacterSet>\n"
			<< "    <UseDebugLibraries>" << (debug ? "true" : "false") << "</UseDebugLibraries>\n"
			<< "  </PropertyGroup>\n"
			<< "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n"
			<< "  <PropertyGroup>\n"
			<< "    <OutDir>$(ProjectDir)Bin\\$(Configuration)\\</OutDir>\n"
			<< "    <IntDir>$(ProjectDir)Obj\\$(Configuration)\\</IntDir>\n"
			<< "    <TargetName>lion-game</TargetName>\n"
			<< "  </PropertyGroup>\n"
			<< "  <ItemDefinitionGroup>\n"
			<< "    <ClCompile>\n"
			<< "      <AdditionalIncludeDirectories>" << includes << "%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n"
			<< "      <PreprocessorDefinitions>" << defines << ";%(PreprocessorDefinitions)</PreprocessorDefinitions>\n"
			<< "      <LanguageStandard>stdcpp20</LanguageStandard>\n"
			<< "      <RuntimeLibrary>" << (debug ? "MultiThreadedDebugDLL" : "MultiThreadedDLL") << "</RuntimeLibrary>\n"
			<< "      <Optimization>" << (debug ? "Disabled" : "MaxSpeed") << "</Optimization>\n"
			<< "      <DebugInformationFormat>" << (debug ? "ProgramDatabase" : "None") << "</DebugInformationFormat>\n"
			<< "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n"
			<< "    </ClCompile>\n"
			<< "    <Link>\n"
			<< "      <AdditionalDependencies>lion-core.lib;%(AdditionalDependencies)</AdditionalDependencies>\n"
			<< "      <AdditionalLibraryDirectories>" << LibraryDirectory(sdkDirectory).generic_string() << ";%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n"
			<< "      <GenerateDebugInformation>" << (debug ? "true" : "false") << "</GenerateDebugInformation>\n"
			<< "    </Link>\n"
			<< "  </ItemDefinitionGroup>\n"
			<< "  <ItemGroup>\n";

		for (const std::filesystem::path& source : compile)
			vcxproj << "    <ClCompile Include=\"" << source.generic_string() << "\" />\n";

		vcxproj << "  </ItemGroup>\n  <ItemGroup>\n";

		for (const std::filesystem::path& header : include)
			vcxproj << "    <ClInclude Include=\"" << header.generic_string() << "\" />\n";

		vcxproj
			<< "  </ItemGroup>\n"
			<< "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n"
			<< "</Project>\n";

		vcxproj.close();

		// --- The solution, once: it is the door Visual Studio opens the project by, and it never changes.
		const std::filesystem::path solution = project / (Projects::DisplayName(project) + ".sln");

		if (!std::filesystem::exists(solution, code))
		{
			std::ofstream sln(solution, std::ios::trunc);

			sln
				<< "Microsoft Visual Studio Solution File, Format Version 12.00\n"
				<< "# Visual Studio Version 17\n"
				<< "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"Game\", \"Build\\lion-game.vcxproj\", \""
				<< kProjectGuid << "\"\n"
				<< "EndProject\n"
				<< "Global\n"
				<< "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n"
				<< "\t\tDebug|x64 = Debug|x64\n"
				<< "\t\tRelease|x64 = Release|x64\n"
				<< "\t\tShipping|x64 = Shipping|x64\n"
				<< "\tEndGlobalSection\n"
				<< "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n"
				<< "\t\t" << kProjectGuid << ".Debug|x64.ActiveCfg = Debug|x64\n"
				<< "\t\t" << kProjectGuid << ".Debug|x64.Build.0 = Debug|x64\n"
				<< "\t\t" << kProjectGuid << ".Release|x64.ActiveCfg = Release|x64\n"
				<< "\t\t" << kProjectGuid << ".Release|x64.Build.0 = Release|x64\n"
				<< "\t\t" << kProjectGuid << ".Shipping|x64.ActiveCfg = Shipping|x64\n"
				<< "\t\t" << kProjectGuid << ".Shipping|x64.Build.0 = Shipping|x64\n"
				<< "\tEndGlobalSection\n"
				<< "EndGlobal\n";
		}

		// Older generated solutions listed only the configuration of the editor that created them.
		// Add a missing configuration without replacing other projects or user-authored solution settings.
		std::ifstream existing(solution);
		std::string contents((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
		const std::string platform = configuration + "|x64";
		if (contents.find(platform + " =") == std::string::npos)
		{
			const auto add = [&](const std::string& section, const std::string& lines) -> bool
			{
				const size_t start = contents.find("GlobalSection(" + section + ")");
				const size_t end = start == std::string::npos ? start : contents.find("\tEndGlobalSection", start);
				if (end == std::string::npos) return false;
				contents.insert(end, lines);
				return true;
			};
			if (!add("SolutionConfigurationPlatforms", "\t\t" + platform + " = " + platform + "\n")
				|| !add("ProjectConfigurationPlatforms", "\t\t" + std::string(kProjectGuid) + "." + platform + ".ActiveCfg = " + platform
					+ "\n\t\t" + kProjectGuid + "." + platform + ".Build.0 = " + platform + "\n"))
			{
				error = "The project solution is missing its build configuration sections.";
				return false;
			}
			existing.close();
			std::ofstream updated(solution, std::ios::trunc);
			updated << contents;
			updated.close();
			if (!updated) { error = "Could not update the project solution configuration."; return false; }
		}

		return true;
	}

	Lion::int32 RunCommand(const std::string& command, std::string& output)
	{
#ifdef LN_PLATFORM_WIN
		SECURITY_ATTRIBUTES security = {};
		security.nLength = sizeof(security);
		security.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;

		if (!CreatePipe(&readPipe, &writePipe, &security, 0))
			return -1;

		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOA startup = {};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.hStdOutput = writePipe;
		startup.hStdError = writePipe;

		std::string line = "cmd.exe /C \"" + command + "\"";
		PROCESS_INFORMATION process = {};
		const BOOL started = CreateProcessA(nullptr, line.data(), nullptr, nullptr, TRUE,
			CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);

		CloseHandle(writePipe);

		if (!started)
		{
			CloseHandle(readPipe);
			return -1;
		}

		char8 buffer[512];
		DWORD read = 0;

		while (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0)
			output.append(buffer, read);

		WaitForSingleObject(process.hProcess, INFINITE);

		DWORD exitCode = 0;
		GetExitCodeProcess(process.hProcess, &exitCode);
		CloseHandle(readPipe);
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		return static_cast<Lion::int32>(exitCode);
#else
		(void)command;
		(void)output;
		return -1;
#endif
	}

	const std::string& MSBuildPath()
	{
		static const std::string path = []
		{
			char8* programFiles = nullptr;
			size_t length = 0;

			if (_dupenv_s(&programFiles, &length, "ProgramFiles(x86)") != 0 || !programFiles)
				return std::string();

			const std::string vswhere = "\"" + std::string(programFiles)
				+ "\\Microsoft Visual Studio\\Installer\\vswhere.exe\"";
			std::free(programFiles);
			std::string output;

			if (RunCommand(vswhere + " -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe", output) != 0)
				return std::string();

			const size_t end = output.find_first_of("\r\n");
			return end == std::string::npos ? output : output.substr(0, end);
		}();

		return path;
	}

	bool Build(const std::filesystem::path& project, const std::string& configuration,
		const std::filesystem::path& sdkDirectory, std::string& output, std::string& error)
	{
		if (!HasNativeSources(project))
		{
			std::error_code code;
			const auto module = ModulePath(project, configuration);
			std::filesystem::create_directories(module.parent_path(), code);
			if (!code) std::filesystem::copy_file(sdkDirectory / "ScenePlayer" / kGameModuleFile,
				module, std::filesystem::copy_options::overwrite_existing, code);
			if (code) { error = "Could not install the standard scene player: " + code.message(); return false; }
			output += "Using the packaged scene player; no C++ compiler is required.\n";
			error.clear();
			return true;
		}
		if (MSBuildPath().empty())
		{
			error = "Could not locate MSBuild; install Visual Studio with the C++ tools.";
			return false;
		}

		if (!Generate(project, configuration, sdkDirectory, error))
			return false;

		const std::string command =
			"\"" + MSBuildPath() + "\""
			" \"" + (project / (Projects::DisplayName(project) + ".sln")).string() + "\""
			" -p:PlatformToolset=" + PlatformToolset() +
			" -p:Configuration=" + configuration +
			" -p:Platform=x64 -v:minimal -nologo -nodeReuse:false";

		const int32 exitCode = RunCommand(command, output);

		if (exitCode != 0)
		{
			error = "The game module build failed.";
			return false;
		}

		return true;
	}
}
