#include "EditorPch.h"
#include "Projects.h"

#include "ProjectBuild.h"

#include <Lion/Core/CommandLine.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Vault.h>
#include <Lion/Platform/FileAssociation.h>

#include <ctime>

#include <nlohmann/json.hpp>

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

		std::filesystem::path FavoriteFile()
		{
			return EditorDataDirectory() / "favorite-projects.txt";
		}

		// One path per line, trimmed, empties dropped — the shape of every list the manager keeps.
		std::vector<std::string> ReadLines(const std::filesystem::path& file)
		{
			std::vector<std::string> lines;
			std::ifstream stream(file);
			std::string line;

			while (std::getline(stream, line))
			{
				while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
					line.pop_back();

				if (!line.empty())
					lines.push_back(line);
			}

			return lines;
		}

		void WriteLines(const std::filesystem::path& file, const std::vector<std::string>& lines)
		{
			std::error_code error;
			std::filesystem::create_directories(EditorDataDirectory(), error);

			std::ofstream stream(file, std::ios::trunc);

			for (const std::string& line : lines)
				stream << line << '\n';
		}

		// The raw recents as written, no validation: the list edits work on it as it is on disk, while
		// LoadRecent filters what is offered.
		std::vector<std::string> ReadRecentLines()
		{
			return ReadLines(RecentFile());
		}

		void WriteRecentLines(const std::vector<std::string>& lines)
		{
			WriteLines(RecentFile(), lines);
		}

		// The project's marker, or nothing when it carries none (the built-in, or a bare Assets folder).
		std::filesystem::path MarkerPath(const std::filesystem::path& project)
		{
			std::error_code error;

			for (const auto& entry : std::filesystem::directory_iterator(project, error))
				if (entry.path().extension() == kFileExtension)
					return entry.path();

			return {};
		}

		// The name becomes a folder and a file, so it is held to what a folder and a file can carry.
		bool IsValidName(const std::string& name)
		{
			return !name.empty() && name.find_first_not_of(
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 _-") == std::string::npos;
		}

		// Writes the marker naming the project, replacing whichever one it had. It goes through the same
		// JSON library the engine serializes scenes with — not a string with braces in it, which is a
		// serializer waiting to disagree with the real one — and it leaves sealed, the way a scene does:
		// the engine's files wear the engine's seal, whichever kind they are.
		void WriteMarker(const std::filesystem::path& project, const std::string& name, const std::string& version)
		{
			const std::filesystem::path existing = MarkerPath(project);
			const std::filesystem::path renamed = project / (name + kFileExtension);

			std::error_code error;

			if (!existing.empty() && existing != renamed)
				std::filesystem::remove(existing, error);

			nlohmann::json content;
			content["name"] = name;
			content["engine"] = version;

			std::ofstream marker(renamed);
			marker << Lion::Vault::Seal(content.dump(2));
		}

		// A folder's last-write moment as "YYYY-MM-DD HH:MM", local time — the column Godot's manager keeps.
		std::string FormatStamp(std::filesystem::file_time_type stamp)
		{
			using namespace std::chrono;

			const std::time_t time = system_clock::to_time_t(clock_cast<system_clock>(stamp));
			std::tm local = {};
			localtime_s(&local, &time);

			Lion::char8 buffer[24] = {};
			std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local);
			return buffer;
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
		// Beside the executable when that folder takes writes — a development tree, a portable unzip —
		// and in the user's local app data when it does not: an editor installed where Windows guards
		// the folder runs read-only, which is why Unity, Unreal and Godot all keep state in the user's
		// own corner. Probed once with a real write, because only a write answers whether writes work.
		static const std::filesystem::path directory = []() -> std::filesystem::path
		{
			const std::filesystem::path beside = std::filesystem::path(Lion::ResourceRootDirectory()) / "Data";

			std::error_code error;
			std::filesystem::create_directories(beside, error);

			const std::filesystem::path probe = beside / ".writable";

			if (std::ofstream(probe).put('\n'))
			{
				std::filesystem::remove(probe, error);
				return beside;
			}

			const Lion::char8* local = std::getenv("LOCALAPPDATA");
			const std::filesystem::path fallback = (local && *local)
				? std::filesystem::path(local) / "Lion Engine" / "Data"
				: std::filesystem::temp_directory_path(error) / "Lion Engine" / "Data";

			std::filesystem::create_directories(fallback, error);
			return fallback;
		}();

		return directory;
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

		const std::filesystem::path marker = MarkerPath(project);
		return marker.empty() ? project.filename().generic_string() : marker.stem().generic_string();
	}

	std::string EngineVersion(const std::filesystem::path& project)
	{
		const std::filesystem::path marker = MarkerPath(project);

		if (marker.empty())
			return {};

		// Read the way it was written — unsealed, then the JSON library, not a string search. Unseal gives
		// plain text back unchanged, so a marker written by hand still reads; a marker edited into
		// something else records nothing.
		std::ifstream file(marker);
		std::stringstream buffer;
		buffer << file.rdbuf();

		try
		{
			const nlohmann::json content = nlohmann::json::parse(Lion::Vault::Unseal(buffer.str()));
			return content.value("engine", std::string());
		}
		catch (const std::exception&)
		{
			return {};
		}
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
		// Checked here, where the scaffolding happens, whatever UI asked for it.
		if (!IsValidName(name))
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
		WriteMarker(folder, name, Lion::kVersion);

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

		// The project is born with its Visual Studio files, the way an Unreal game is: a solution at its
		// root, tied to the SDK beside the editor, openable in VS from the first minute. Best-effort — the
		// compile path regenerates the project file anyway, and an editor without an SDK scaffolds the
		// rest all the same.
		std::string buildError;
		ProjectBuild::Generate(folder, buildError);

		return folder;
	}

	std::vector<Entry> LoadAll()
	{
		std::vector<std::string> paths = ReadRecentLines();

		const std::filesystem::path builtIn = DefaultProjectDirectory();
		const std::string builtInPath = builtIn.generic_string();

		if (!builtIn.empty() && IsProjectFolder(builtIn)
			&& std::find(paths.begin(), paths.end(), builtInPath) == paths.end())
			paths.push_back(builtInPath);

		const std::vector<std::string> favorites = ReadLines(FavoriteFile());

		std::vector<Entry> entries;
		entries.reserve(paths.size());

		for (const std::string& path : paths)
		{
			Entry entry;
			entry.path = path;
			entry.builtIn = (path == builtInPath);
			entry.missing = !IsProjectFolder(path);
			entry.name = DisplayName(path);
			entry.favorite = std::find(favorites.begin(), favorites.end(), path) != favorites.end();

			if (!entry.missing)
			{
				// The built-in carries no marker; the engine it was made with is the one running.
				entry.version = entry.builtIn ? Lion::kVersion : EngineVersion(path);

				std::error_code error;
				entry.editedAt = std::filesystem::last_write_time(path, error);

				if (!error)
					entry.edited = FormatStamp(entry.editedAt);
			}

			entries.push_back(entry);
		}

		return entries;
	}

	void SetFavorite(const std::filesystem::path& folder, bool favorite)
	{
		const std::string path = folder.generic_string();

		std::vector<std::string> favorites = ReadLines(FavoriteFile());
		favorites.erase(std::remove(favorites.begin(), favorites.end(), path), favorites.end());

		if (favorite)
			favorites.push_back(path);

		WriteLines(FavoriteFile(), favorites);
	}

	void RemoveRecent(const std::filesystem::path& folder)
	{
		const std::string path = folder.generic_string();

		std::vector<std::string> lines = ReadRecentLines();
		lines.erase(std::remove(lines.begin(), lines.end(), path), lines.end());
		WriteRecentLines(lines);

		SetFavorite(folder, false);
	}

	void RemoveMissing()
	{
		std::vector<std::string> lines = ReadRecentLines();

		lines.erase(std::remove_if(lines.begin(), lines.end(),
			[](const std::string& line) { return !IsProjectFolder(line); }), lines.end());

		WriteRecentLines(lines);
	}

	bool Rename(const std::filesystem::path& folder, const std::string& newName, std::string& error)
	{
		if (!IsValidName(newName))
		{
			error = "A project name takes letters, digits, spaces, - and _ only.";
			return false;
		}

		if (!IsProjectFolder(folder))
		{
			error = "The project is not on disk to rename.";
			return false;
		}

		// The version the old marker recorded survives the new name; a project without one was made by no
		// recorded engine until now, so it takes the running one.
		const std::string version = EngineVersion(folder);
		WriteMarker(folder, newName, version.empty() ? std::string(Lion::kVersion) : version);
		return true;
	}

	std::filesystem::path Duplicate(const std::filesystem::path& folder, std::string& error)
	{
		if (!IsProjectFolder(folder))
		{
			error = "The project is not on disk to duplicate.";
			return {};
		}

		// A sibling named after the original, counted up until the name is free.
		const std::filesystem::path parent = folder.parent_path();
		const std::string base = DisplayName(folder) + " - Copy";

		std::error_code code;
		std::filesystem::path destination = parent / base;

		for (Lion::int32 counter = 2; std::filesystem::exists(destination, code); ++counter)
			destination = parent / (base + " " + std::to_string(counter));

		std::filesystem::copy(folder, destination, std::filesystem::copy_options::recursive, code);

		if (code)
		{
			error = code.message();
			return {};
		}

		// The copy is its own project, named after its folder rather than the one it came from.
		std::string renameError;
		Rename(destination, destination.filename().generic_string(), renameError);

		Remember(destination);
		return destination;
	}

	void RegisterFileType()
	{
		const std::filesystem::path executable = Lion::CommandLine::Get(0);

		if (executable.empty())
			return;

		// The icon is the one shipped beside the executable rather than the executable's own, so a project
		// in Explorer looks like a project and not like a second copy of the editor. Native separators:
		// this string is read by Explorer, not by the engine.
		std::filesystem::path icon = executable.parent_path() / Lion::kEngineIconResource;
		icon.make_preferred();

		// The identifier is its own name, not the extension's, so retiring an extension retires its keys
		// without touching these.
		Lion::FileAssociation::Register(".lnproject", "Lion.EngineProject", "Lion Project",
			icon.string(), executable.string());

		// Claims older versions wrote are taken back: scenes open inside the editor, the way they do in
		// Unreal and Visual Studio, and the marker moved from .lproject to .lnproject.
		Lion::FileAssociation::Unregister(".lscene", "Lion.Scene");
		Lion::FileAssociation::Unregister(".lproject", "Lion.Project");
	}
}
