#include "EditorPch.h"
#include "Projects.h"

#include <Lion/Core/Filesystem.h>

namespace Projects
{
	namespace
	{
		// The game's folder inside the engine tree, relative to its root — the folder, not the VS project
		// name. It is what the walk below recognises the root by.
		constexpr const Lion::char8* kGameFolder = "Sandbox";

		std::filesystem::path RecentFile()
		{
			return EditorDataDirectory() / "recent-projects.txt";
		}

		// The raw recents as written, no validation: Remember edits the list as it is on disk, while
		// LoadRecent filters what is offered.
		std::vector<std::string> ReadRecentLines()
		{
			std::vector<std::string> lines;
			std::ifstream file(RecentFile());
			std::string line;

			while (std::getline(file, line))
			{
				while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
					line.pop_back();

				if (!line.empty())
					lines.push_back(line);
			}

			return lines;
		}

		void WriteRecentLines(const std::vector<std::string>& lines)
		{
			std::error_code error;
			std::filesystem::create_directories(EditorDataDirectory(), error);

			std::ofstream file(RecentFile(), std::ios::trunc);

			for (const std::string& line : lines)
				file << line << '\n';
		}
	}

	std::filesystem::path EngineRootDirectory()
	{
		// Found once and kept: the tree does not move while the editor runs, and the walk is a handful of
		// filesystem probes.
		static const std::filesystem::path root = []() -> std::filesystem::path
		{
			std::filesystem::path current = Lion::ResourceRootDirectory();

			// The resource root keeps a trailing separator, which leaves an empty final component; drop it
			// so the first climb reaches the parent directory and not this same one again.
			if (!current.has_filename())
				current = current.parent_path();

			std::error_code error;

			for (Lion::int32 depth = 0; depth < 8; ++depth)
			{
				if (std::filesystem::is_directory(current / kGameFolder / "Source", error))
					return current;

				if (!current.has_parent_path() || current.parent_path() == current)
					break;

				current = current.parent_path();
			}

			return {};
		}();

		return root;
	}

	std::filesystem::path DefaultProjectDirectory()
	{
		const std::filesystem::path root = EngineRootDirectory();
		return root.empty() ? root : root / kGameFolder;
	}

	std::filesystem::path EditorDataDirectory()
	{
		return std::filesystem::path(Lion::ResourceRootDirectory()) / "Data";
	}

	bool IsProjectFolder(const std::filesystem::path& folder)
	{
		std::error_code error;
		return std::filesystem::is_directory(folder / "Assets", error);
	}

	std::string DisplayName(const std::filesystem::path& project)
	{
		if (project.empty())
			return {};

		std::error_code error;

		for (const auto& entry : std::filesystem::directory_iterator(project, error))
			if (entry.path().extension() == kFileExtension)
				return entry.path().stem().generic_string();

		return project.filename().generic_string();
	}

	std::string EngineVersion(const std::filesystem::path& project)
	{
		std::error_code error;

		for (const auto& entry : std::filesystem::directory_iterator(project, error))
		{
			if (entry.path().extension() != kFileExtension)
				continue;

			// The marker is a couple of lines of JSON; a whole parser for one quoted value would be a
			// dependency for a label. Find the key, take what sits between the next pair of quotes.
			std::ifstream file(entry.path());
			const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			const size_t key = text.find("\"engine\"");

			if (key == std::string::npos)
				break;

			const size_t open = text.find('"', text.find(':', key));
			const size_t close = open == std::string::npos ? std::string::npos : text.find('"', open + 1);

			if (close != std::string::npos)
				return text.substr(open + 1, close - open - 1);

			break;
		}

		return {};
	}

	std::vector<std::string> LoadRecent()
	{
		std::vector<std::string> projects;

		// A project moved or deleted since it was last opened is dropped, not offered as a dead end.
		for (const std::string& line : ReadRecentLines())
			if (IsProjectFolder(line))
				projects.push_back(line);

		// The built-in is always on the list when it is around: it is the one project never opened through
		// the manager, and leaving it once must not leave the folder browser as the only way back. At the
		// end, not the front — recency belongs to what was actually opened.
		const std::filesystem::path builtIn = DefaultProjectDirectory();

		if (!builtIn.empty() && IsProjectFolder(builtIn))
		{
			const std::string path = builtIn.generic_string();

			if (std::find(projects.begin(), projects.end(), path) == projects.end())
				projects.push_back(path);
		}

		return projects;
	}

	void Remember(const std::filesystem::path& folder)
	{
		const std::string path = folder.generic_string();

		std::vector<std::string> lines = ReadRecentLines();
		lines.erase(std::remove(lines.begin(), lines.end(), path), lines.end());
		lines.insert(lines.begin(), path);

		constexpr size_t kMaxRecent = 10;

		if (lines.size() > kMaxRecent)
			lines.resize(kMaxRecent);

		WriteRecentLines(lines);
	}

	std::filesystem::path CreateOnDisk(const std::string& name, const std::filesystem::path& location, std::string& error)
	{
		// The name becomes a folder and a file, so it is held to what a folder and a file can carry —
		// checked here, where the scaffolding happens, whatever UI asked for it.
		if (name.empty() || name.find_first_not_of(
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 _-") != std::string::npos)
		{
			error = "A project name takes letters, digits, spaces, - and _ only.";
			return {};
		}

		const std::filesystem::path folder = location / name;

		std::error_code code;

		if (std::filesystem::exists(folder, code))
		{
			error = "'" + folder.generic_string() + "' already exists.";
			return {};
		}

		// The shape of a Lion project: assets to browse, a place for scenes, and a source tree for components.
		std::filesystem::create_directories(folder / "Assets" / "Scenes", code);
		std::filesystem::create_directories(folder / "Source", code);

		if (code)
		{
			error = code.message();
			return {};
		}

		// The marker that names the project and records which engine made it.
		std::ofstream marker(folder / (name + kFileExtension));
		marker << "{\n\t\"name\": \"" << name << "\",\n\t\"engine\": \"" << Lion::kVersion << "\"\n}\n";
		marker.close();

		// A game module the editor and launcher can load: the exported entry point every project needs,
		// empty but complete, so the project compiles from the first build. Components added to it are
		// picked up by the glob — a game grows by adding files, not by editing this one.
		std::ofstream module(folder / "Source" / "GameModule.cpp");
		module
			<< "#include <Lion/Lion.h>\n\n"
			<< "// The game module's entry point: the launcher and the editor both load this library and call\n"
			<< "// this to build the game. Push your layers onto the application as the game grows; the components\n"
			<< "// under Assets/Scripts register themselves by being compiled in, and need no mention here.\n"
			<< "extern \"C\" __declspec(dllexport) Lion::Application* LionCreateApplication()\n"
			<< "{\n"
			<< "\treturn new Lion::Application();\n"
			<< "}\n";
		module.close();

		return folder;
	}
}
