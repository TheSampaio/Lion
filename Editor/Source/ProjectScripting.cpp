#include "EditorPch.h"
#include "ProjectScripting.h"

#include "ProjectBuild.h"

#include <Lion/Core/Log.h>
#include <Lion/Scripting/CSharpRuntime.h>

#include <chrono>
#include <cstdlib>
#include <sstream>

namespace ProjectScripting
{
	namespace
	{
		std::filesystem::path sPreviousProject;
		std::filesystem::path sPreviousAssembly;

		std::string Xml(const std::string& value)
		{
			std::string escaped;
			for (const char character : value)
			{
				switch (character)
				{
					case '&': escaped += "&amp;"; break;
					case '<': escaped += "&lt;"; break;
					case '>': escaped += "&gt;"; break;
					case '"': escaped += "&quot;"; break;
					default: escaped += character; break;
				}
			}
			return escaped;
		}

		std::filesystem::path HostfxrPath()
		{
			std::vector<std::filesystem::path> roots;
			for (const char* name : { "DOTNET_ROOT_X64", "DOTNET_ROOT", "ProgramFiles" })
			{
				char* value = nullptr;
				size_t length = 0;
				if (_dupenv_s(&value, &length, name) == 0 && value)
				{
					roots.emplace_back(std::string(name) == "ProgramFiles" ? std::filesystem::path(value) / "dotnet" : value);
					std::free(value);
				}
			}
			for (const auto& root : roots)
			{
				std::error_code code;
				std::pair<int, int> newest(-1, -1);
				std::filesystem::path chosen;
				for (const auto& entry : std::filesystem::directory_iterator(root / "host" / "fxr", code))
				{
					int major = 0, minor = 0, patch = 0;
					char dot1 = 0, dot2 = 0;
					std::istringstream version(entry.path().filename().string());
					if (!(version >> major >> dot1 >> minor >> dot2 >> patch) || major != 10
						|| dot1 != '.' || dot2 != '.' || version.peek() != EOF)
						continue;
					const std::pair<int, int> candidate(minor, patch);
					if (candidate > newest && std::filesystem::is_regular_file(entry.path() / "hostfxr.dll", code))
					{
						newest = candidate;
						chosen = entry.path() / "hostfxr.dll";
					}
				}
				if (!chosen.empty())
					return std::filesystem::absolute(chosen);
			}
			return {};
		}
	}

	bool HasSources(const std::filesystem::path& project)
	{
		std::error_code code;
		const auto assets = project / "Assets";
		for (std::filesystem::recursive_directory_iterator it(assets, code), end; it != end; it.increment(code))
		{
			if (code)
				break;
			if (it->is_regular_file(code) && it->path().extension() == ".cs")
				return true;
		}
		return false;
	}

	std::filesystem::path AssemblyPath(const std::filesystem::path& project, const std::string& configuration)
	{
		return project / "Build" / "Managed" / configuration / "lion-scripts.dll";
	}

	bool Build(const std::filesystem::path& project, const std::string& configuration,
		const std::filesystem::path& sdkDirectory, std::string& output, std::string& error)
	{
		if (!HasSources(project))
			return true;
		if (configuration != "Debug" && configuration != "Release" && configuration != "Shipping")
		{
			error = "Unsupported managed build configuration.";
			return false;
		}
		std::error_code code;
		const auto api = std::filesystem::absolute(sdkDirectory / "Managed" / "Lion.Engine.dll", code);
		if (code || !std::filesystem::is_regular_file(api, code))
		{
			error = "The editor's Managed SDK is missing. Build or install tools with the .NET 10 SDK available.";
			return false;
		}
		const auto build = project / "Build";
		std::filesystem::create_directories(build, code);
		if (code) { error = code.message(); return false; }
		const auto csproj = build / "lion-scripts.csproj";
		std::ofstream file(csproj, std::ios::trunc);
		if (!file) { error = "Could not write the managed project build."; return false; }
		file << "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
			<< "  <PropertyGroup>\n"
			<< "    <TargetFramework>net10.0</TargetFramework>\n"
			<< "    <AssemblyName>lion-scripts</AssemblyName>\n"
			<< "    <Nullable>enable</Nullable>\n"
			<< "    <ImplicitUsings>enable</ImplicitUsings>\n"
			<< "    <Deterministic>true</Deterministic>\n"
			<< "    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>\n"
			<< "    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>\n"
			<< "    <OutputPath>Managed/" << configuration << "/</OutputPath>\n"
			<< "  </PropertyGroup>\n"
			<< "  <ItemGroup>\n"
			<< "    <Compile Include=\"../Assets/**/*.cs\" />\n"
			<< "    <Reference Include=\"Lion.Engine\"><HintPath>" << Xml(api.generic_string())
			<< "</HintPath><Private>false</Private></Reference>\n"
			<< "  </ItemGroup>\n</Project>\n";
		file.close();
		if (!file) { error = "Could not finish the managed project build."; return false; }
		const std::string command = "dotnet build \"" + csproj.string() + "\" --configuration "
			+ (configuration == "Debug" ? "Debug" : "Release") + " --nologo --ignore-failed-sources -p:NuGetAudit=false";
		if (ProjectBuild::RunCommand(command, output) != 0)
		{
			error = "C# compilation failed. Install the .NET 10 SDK and inspect compiler diagnostics.";
			return false;
		}
		error.clear();
		return true;
	}

	bool Load(const std::filesystem::path& project, const std::string& configuration,
		const std::filesystem::path& sdkDirectory, std::string& error)
	{
		Lion::CSharpRuntime::SetGameplayActive(false);
		if (!HasSources(project))
			return true;
		std::error_code code;
		const auto source = AssemblyPath(project, configuration);
		if (!std::filesystem::is_regular_file(source, code))
		{
			error = "Compile this project's C# scripts before entering Play.";
			return false;
		}
		const auto hostfxr = HostfxrPath();
		if (hostfxr.empty())
		{
			error = "C# gameplay requires the Windows x64 .NET 10 runtime. Install the SDK to compile scripts too.";
			return false;
		}
		const auto api = std::filesystem::absolute(sdkDirectory / "Managed", code);
		if (code || !Lion::CSharpRuntime::Initialize(api.string(), hostfxr.string(), error))
			return false;

		// A private copy lets compilation replace the original while its collectible context is alive.
		static Lion::uint64 nextSession = 0;
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		const auto loaded = project / "Build" / "ManagedLoaded" / (std::to_string(stamp) + "-" + std::to_string(++nextSession));
		std::filesystem::create_directories(loaded, code);
		if (!code)
			std::filesystem::copy(source.parent_path(), loaded,
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, code);
		if (code || !Lion::CSharpRuntime::LoadAssembly(std::filesystem::absolute(loaded / source.filename()).string(), error))
		{
			if (code) error = "Could not create the private managed build copy: " + code.message();
			const std::string failure = error;
			std::string ignored;
			Lion::CSharpRuntime::Shutdown(ignored);
			// A compilable assembly can still have invalid authoring metadata. Retain the last catalog
			// for this same project so reconstruction does not discard its authored script components.
			if (sPreviousProject == project.lexically_normal() && !sPreviousAssembly.empty()
				&& Lion::CSharpRuntime::Initialize(api.string(), hostfxr.string(), ignored)
				&& Lion::CSharpRuntime::LoadAssembly(sPreviousAssembly.string(), ignored))
				error = failure + " The previous C# catalog was restored.";
			else
			{
				Lion::CSharpRuntime::Shutdown(ignored);
				error = failure;
			}
			return false;
		}
		sPreviousProject = project.lexically_normal();
		sPreviousAssembly = std::filesystem::absolute(loaded / source.filename());
		Lion::Log::Console(Lion::LogLevel::Success, "[Editor] Loaded the C# script catalog.");
		return true;
	}

	void Unload()
	{
		std::string error;
		if (!Lion::CSharpRuntime::Shutdown(error))
			Lion::Log::Console(Lion::LogLevel::Error, "[Editor] C# shutdown failed: " + error);
	}
}
