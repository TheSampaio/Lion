#include "EditorPch.h"
#include "ProjectManagerLayer.h"

#include "../EditorGui.h"
#include "../Projects.h"

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

using namespace Lion;

namespace
{
	// One project's row in the list: tall enough for a name over a path, the way Godot draws them.
	constexpr float32 kRowHeight = 48.0f;

	// Starts this same executable again with the given arguments, detached — the manager does not wait for
	// the editor it starts, it gets out of its way.
	void LaunchSelf(const std::string& arguments)
	{
#ifdef LN_PLATFORM_WIN
		char executable[MAX_PATH] = { 0 };
		GetModuleFileNameA(nullptr, executable, MAX_PATH);

		// CreateProcess may scribble on the command line, so it gets its own mutable copy.
		std::string line = "\"" + std::string(executable) + "\" " + arguments;

		STARTUPINFOA startup = {};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process = {};

		if (CreateProcessA(nullptr, line.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process))
		{
			CloseHandle(process.hThread);
			CloseHandle(process.hProcess);
		}
		else
		{
			Log::Console(LogLevel::Error, "[ProjectManager] Could not start the editor.");
		}
#else
		(void)arguments;
#endif
	}
}

void ProjectManagerLayer::OnAttach()
{
	// A picker, not a workspace: a modest window with the caption Windows draws — the editor's custom
	// chrome belongs to the editor. It opens windowed and centred, never maximised.
	Window::SetSize(1000, 640);
	Window::SetTitle("Lion Engine - Project Manager");
	Window::SetBackgroundColor(0.10f, 0.10f, 0.11f);
	Window::SetResizable(true);
	Window::SetMaximized(false);
	Window::SetDarkTitleBar(true);
}

void ProjectManagerLayer::OnCreate()
{
	EditorGui::Init();

	mProjects = Projects::LoadRecent();
	mLogo = Texture::Create(kEngineIconFile, TextureFilter::Linear);
}

void ProjectManagerLayer::OnRender()
{
	EditorGui::BeginFrame();
	DrawManager();
	EditorGui::EndFrame();
}

void ProjectManagerLayer::OnDetach()
{
	EditorGui::Shutdown();
}

void ProjectManagerLayer::DrawManager()
{
	// One window filling the frame: the manager is this page, not a panel arrangement.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::Begin("##project_manager", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

	// --- Header: the engine's mark, the page's name, and the filter.
	const float32 logoSize = 40.0f;

	if (mLogo)
		ImGui::Image(static_cast<ImTextureID>(mLogo->GetNativeHandle()), ImVec2(logoSize, logoSize));

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::PushFont(EditorGui::GetBoldFont());
	ImGui::TextUnformatted("Lion Engine");
	ImGui::PopFont();
	ImGui::TextDisabled("Project Manager");
	ImGui::EndGroup();

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 260.0f);
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputTextWithHint("##filter", ICON_MDI_MAGNIFY "  Filter projects", mFilter, sizeof(mFilter));

	ImGui::Spacing();

	// --- Toolbar.
	if (ImGui::Button(ICON_MDI_PLUS "  New Project", ImVec2(140.0f, 0.0f)))
	{
		mCreating = !mCreating;
		mCreateError.clear();
	}

	ImGui::SameLine();

	if (ImGui::Button(ICON_MDI_FOLDER_OPEN "  Open Folder...", ImVec2(150.0f, 0.0f)))
	{
		const std::string picked = FileDialog::OpenFolder(std::string());

		if (!picked.empty())
		{
			if (Projects::IsProjectFolder(picked))
				OpenInEditor(picked);
			else
				mCreateError = "'" + picked + "' is not a Lion project (no Assets folder).";
		}
	}

	if (!mCreateError.empty())
		ImGui::TextColored(ImVec4(0.90f, 0.35f, 0.35f, 1.0f), "%s", mCreateError.c_str());

	ImGui::Spacing();

	if (mCreating)
		DrawCreateSection();

	// --- The projects, then the version in the corner every engine keeps it in.
	const float32 footer = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;

	ImGui::BeginChild("##projects", ImVec2(0.0f, -footer), ImGuiChildFlags_None);
	DrawProjectList();
	ImGui::EndChild();

	ImGui::Separator();
	const float32 versionWidth = ImGui::CalcTextSize(kVersion).x;
	ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - versionWidth + ImGui::GetCursorPosX());
	ImGui::TextDisabled("%s", kVersion);

	ImGui::End();
}

void ProjectManagerLayer::DrawProjectList()
{
	const std::string filter = mFilter;
	std::string openRequest;

	for (const std::string& path : mProjects)
	{
		const std::string name = Projects::DisplayName(path);

		// The filter reads the name the row shows, case blind: a filter that only matches exact case is a
		// filter that mostly finds nothing.
		if (!filter.empty())
		{
			const auto lowered = [](std::string text)
			{
				std::transform(text.begin(), text.end(), text.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return text;
			};

			if (lowered(name).find(lowered(filter)) == std::string::npos)
				continue;
		}

		ImGui::PushID(path.c_str());

		// The row is one selectable the height of both its lines; what it says is drawn over it, so the
		// whole of it is one target to hit.
		const ImVec2 rowMin = ImGui::GetCursorScreenPos();
		const float32 rowWidth = ImGui::GetContentRegionAvail().x;

		if (ImGui::Selectable("##row", false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, kRowHeight))
			&& ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			openRequest = path;

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Double-click to open");

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImU32 nameColor = ImGui::GetColorU32(ImGuiCol_Text);
		const ImU32 pathColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
		const float32 padding = 8.0f;
		const float32 lineHeight = ImGui::GetTextLineHeight();

		// The icon comes from the text font, where the glyphs are merged — the bold cut never got them, so
		// an icon drawn with it would come out as the box a font shows for a glyph it does not have.
		draw->AddText(ImVec2(rowMin.x + padding, rowMin.y + padding), nameColor, ICON_MDI_FOLDER);

		ImFont* bold = EditorGui::GetBoldFont();
		draw->AddText(bold, bold ? bold->FontSize : lineHeight,
			ImVec2(rowMin.x + padding + 26.0f, rowMin.y + padding), nameColor, name.c_str());
		draw->AddText(ImVec2(rowMin.x + padding + 26.0f, rowMin.y + padding + lineHeight + 4.0f), pathColor, path.c_str());

		// Which engine made it, kept to the right the way Godot keeps it.
		const std::string version = Projects::EngineVersion(path);

		if (!version.empty())
		{
			const float32 versionWidth = ImGui::CalcTextSize(version.c_str()).x;
			draw->AddText(ImVec2(rowMin.x + rowWidth - versionWidth - padding,
				rowMin.y + (kRowHeight - lineHeight) * 0.5f), pathColor, version.c_str());
		}

		ImGui::PopID();
	}

	if (mProjects.empty())
		ImGui::TextDisabled("No projects yet — create one above.");

	if (!openRequest.empty())
		OpenInEditor(openRequest);
}

void ProjectManagerLayer::DrawCreateSection()
{
	// The "New project" drawer, open until it creates or is toggled away.
	ImGui::PushFont(EditorGui::GetBoldFont());
	ImGui::TextUnformatted("New project");
	ImGui::PopFont();

	const ImGuiStyle& style = ImGui::GetStyle();
	const float32 browseWidth = ImGui::CalcTextSize(ICON_MDI_FOLDER_OPEN).x + style.FramePadding.x * 2.0f;

	ImGui::SetNextItemWidth(320.0f);
	ImGui::InputTextWithHint("##new_name", "Project name", mName, sizeof(mName));

	ImGui::SetNextItemWidth(320.0f - browseWidth - style.ItemInnerSpacing.x);
	ImGui::InputTextWithHint("##new_location", "Location", mLocation, sizeof(mLocation));

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

	if (ImGui::Button(ICON_MDI_FOLDER_OPEN "##pick_location"))
	{
		const std::string picked = FileDialog::OpenFolder(mLocation);

		if (!picked.empty())
		{
			const size_t copied = picked.copy(mLocation, sizeof(mLocation) - 1);
			mLocation[copied] = '\0';
		}
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(mName[0] == '\0' || mLocation[0] == '\0');

	if (ImGui::Button("Create & Open", ImVec2(130.0f, 0.0f)))
	{
		mCreateError.clear();
		const std::filesystem::path folder = Projects::CreateOnDisk(mName, mLocation, mCreateError);

		if (!folder.empty())
			OpenInEditor(folder.generic_string());
	}

	ImGui::EndDisabled();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

void ProjectManagerLayer::OpenInEditor(const std::string& project)
{
	Projects::Remember(project);

	Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[ProjectManager] Opening '{}'.", Projects::DisplayName(project)));

	// The editor is its own process on this one project; the manager's job ends where the editor's begins.
	LaunchSelf("--project \"" + project + "\"");
	Window::RequestClose();
}
