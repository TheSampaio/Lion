#include "EditorPch.h"
#include "ProjectExporter.h"

#include "ProjectBuild.h"
#include "Projects.h"
#include "Sealer.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/GameModule.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace ProjectExporter
{
	namespace
	{
		bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination,
			std::string& error)
		{
			std::error_code code;
			std::filesystem::create_directories(destination.parent_path(), code);

			if (!code)
				std::filesystem::copy_file(source, destination,
					std::filesystem::copy_options::overwrite_existing, code);

			if (!code)
				return true;

			error = "Could not copy '" + source.generic_string() + "': " + code.message();
			return false;
		}

		bool CopyDirectory(const std::filesystem::path& source, const std::filesystem::path& destination,
			std::string& error)
		{
			std::error_code code;

			if (!std::filesystem::is_directory(source, code))
				return true;

			for (std::filesystem::recursive_directory_iterator it(source, code), end; it != end; it.increment(code))
			{
				if (code)
					break;

				const std::filesystem::path relative = it->path().lexically_relative(source);
				const std::filesystem::path target = destination / relative;

				if (it->is_directory(code))
					std::filesystem::create_directories(target, code);
				else if (it->is_regular_file(code) && !CopyFile(it->path(), target, error))
					return false;
			}

			if (!code)
				return true;

			error = "Could not copy '" + source.generic_string() + "': " + code.message();
			return false;
		}

		bool CopyAssets(const std::filesystem::path& assets, const std::filesystem::path& destination,
			std::string& error)
		{
			std::error_code code;

			for (std::filesystem::recursive_directory_iterator it(assets, code), end; it != end; it.increment(code))
			{
				if (code)
					break;

				if (it->is_directory(code) && it->path().filename() == "Scripts")
				{
					it.disable_recursion_pending();
					continue;
				}

				if (!it->is_regular_file(code))
					continue;

				const std::filesystem::path extension = it->path().extension();

				if (extension == ".cpp" || extension == ".h" || extension == ".hpp" || extension == ".lnexport")
					continue;

				const std::filesystem::path target = destination / it->path().lexically_relative(assets);

				if (!CopyFile(it->path(), target, error))
					return false;
			}

			if (!code)
				return true;

			error = "Could not package the project's assets: " + code.message();
			return false;
		}

	}

	Result ExportWindows(const std::filesystem::path& project, const Options& options)
	{
		Result result;
		const std::filesystem::path& destination = options.destination;
		std::error_code code;
		const std::filesystem::path projectDirectory = std::filesystem::absolute(project, code).lexically_normal();

		if (code)
		{
			result.message = "Could not resolve the active project: " + code.message();
			return result;
		}

		if (!Projects::IsProjectFolder(projectDirectory))
		{
			result.message = "The active project is not available on disk.";
			return result;
		}

		if (destination.empty())
		{
			result.message = "Choose a destination for the exported game.";
			return result;
		}

		const std::filesystem::path root = Projects::EngineRootDirectory();
		const std::filesystem::path runtime = root.empty()
			? std::filesystem::path(Lion::ResourceRootDirectory()) / "ExportTemplates" / "Windows"
			: root / "Build" / "Bin" / "Shipping" / "Launcher";
		const std::filesystem::path sdk = root.empty()
			? std::filesystem::path(Lion::ResourceRootDirectory())
			: root / "Build" / "Bin" / "Shipping" / "Mane";

		const std::filesystem::path launcher = runtime / "lion-launcher.exe";
		if (!std::filesystem::is_regular_file(launcher, code) || !ProjectBuild::Available(sdk))
		{
			result.message = "The Windows Shipping runtime is unavailable. Build the Shipping configuration first.";
			return result;
		}

		std::string buildError;

		if (!ProjectBuild::Build(projectDirectory, "Shipping", sdk, result.buildOutput, buildError))
		{
			result.message = buildError;
			return result;
		}

		const std::string gameName = Projects::DisplayName(projectDirectory);
		std::filesystem::path executableName = options.executableName.empty() ? gameName : options.executableName;
		executableName = executableName.filename();

		if (executableName.extension() == ".exe")
			executableName.replace_extension();

		if (executableName.empty() || executableName == "." || executableName == "..")
		{
			result.message = "Choose a valid executable name.";
			return result;
		}

		const std::filesystem::path output = destination / gameName;
		const std::filesystem::path absoluteProject = projectDirectory;
		const std::filesystem::path absoluteDestination = std::filesystem::absolute(destination, code).lexically_normal();
		const std::filesystem::path destinationInsideProject = absoluteDestination.lexically_relative(absoluteProject);

		if (!destinationInsideProject.empty() && *destinationInsideProject.begin() != "..")
		{
			result.message = "The export destination must be outside the project folder.";
			return result;
		}

		if (std::filesystem::exists(output, code))
		{
			result.message = "'" + output.generic_string() + "' already exists. Choose an empty destination.";
			return result;
		}

		std::filesystem::path staging = destination / (gameName + ".exporting");

		for (Lion::int32 suffix = 2; std::filesystem::exists(staging, code); ++suffix)
			staging = destination / (gameName + ".exporting-" + std::to_string(suffix));

		std::filesystem::create_directories(staging, code);

		if (code)
		{
			result.message = "Could not create the export folder: " + code.message();
			return result;
		}

		const auto fail = [&](const std::string& message)
		{
			std::error_code cleanup;
			std::filesystem::remove_all(staging, cleanup);
			result.message = message;
			return result;
		};

		std::string copyError;

		if (!CopyFile(launcher, staging / (executableName.string() + ".exe"), copyError)
			|| !CopyFile(runtime / "lion-core.dll", staging / "lion-core.dll", copyError)
			|| !CopyFile(runtime / "lion-platform.dll", staging / "lion-platform.dll", copyError)
			|| !CopyFile(ProjectBuild::ModulePath(projectDirectory, "Shipping"), staging / Lion::kGameModuleFile, copyError)
			|| (options.includeIcons && !CopyDirectory(runtime / "Icons", staging / "Icons", copyError))
			|| (options.includeLicenses && !CopyDirectory(runtime / "Licenses", staging / "Licenses", copyError))
			|| !CopyAssets(projectDirectory / "Assets", staging, copyError)
			|| (options.sealAssets && !Sealer::SealAssets(projectDirectory / "Assets", staging, copyError)))
			return fail(copyError);

		std::filesystem::rename(staging, output, code);

		if (code)
			return fail("Could not finish the export: " + code.message());

		result.succeeded = true;
		result.outputDirectory = output;
		result.message = "Exported " + gameName + " for Windows.";
		return result;
	}

	Result ExportWindows(const std::filesystem::path& project, const std::filesystem::path& destination)
	{
		Options options;
		options.destination = destination;
		return ExportWindows(project, options);
	}
}
