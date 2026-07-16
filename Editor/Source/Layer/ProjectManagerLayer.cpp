#include "EditorPch.h"
#include "ProjectManagerLayer.h"

#include "../EditorGui.h"

#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

using namespace Lion;

namespace
{
	// The caption strip: one row, the mark, the name and two buttons — the editor's, minus the row of menus
	// and the maximise a fixed window has no use for.
	constexpr float32 kCaptionHeight = 40.0f;
	constexpr float32 kWindowButton = 46.0f;

	// The banner under it, and the room the content keeps from the edges.
	constexpr float32 kBannerHeight = 150.0f;
	constexpr float32 kPadding = 16.0f;

	// One project's row: tall enough for a name over a path, the way Godot draws them.
	constexpr float32 kRowHeight = 60.0f;

	// The panel of actions beside the list.
	constexpr float32 kActionsWidth = 200.0f;

	// OpenGL keeps its images bottom-up, so everything drawn from a texture flips its V.
	constexpr ImVec2 kFlipTop(0.0f, 1.0f);
	constexpr ImVec2 kFlipBottom(1.0f, 0.0f);

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

	// A five-pointed star, filled or outlined — drawn, not typed: the icon font subset does not carry one,
	// and two more glyphs are not worth risking the atlas for.
	void DrawStar(ImDrawList* draw, const ImVec2& center, float32 radius, bool filled, ImU32 color)
	{
		constexpr int32 kPoints = 5;
		const float32 inner = radius * 0.45f;

		draw->PathClear();

		for (int32 point = 0; point < kPoints * 2; ++point)
		{
			const float32 reach = (point % 2 == 0) ? radius : inner;
			const float32 angle = -IM_PI * 0.5f + point * IM_PI / kPoints;

			draw->PathLineTo(ImVec2(center.x + std::cos(angle) * reach, center.y + std::sin(angle) * reach));
		}

		if (filled)
			draw->PathFillConcave(color);
		else
			draw->PathStroke(color, ImDrawFlags_Closed, 1.0f);
	}
}

void ProjectManagerLayer::OnAttach()
{
	// A picker, not a workspace: a fixed window that cannot be resized or maximised — what would a bigger
	// list of the same projects say? It wears the editor's own caption so the two read as one program.
	Window::SetSize(1100, 700);
	Window::SetTitle("Lion Engine - Project Manager");
	Window::SetBackgroundColor(0.10f, 0.10f, 0.11f);
	Window::SetResizable(false);
	Window::SetMaximized(false);
	Window::SetDarkTitleBar(true);
	Window::SetCustomTitleBar(true, kCaptionHeight);
}

void ProjectManagerLayer::OnCreate()
{
	EditorGui::Init();

	// A .lproject in Explorer opens this program, the way a .uproject opens Unreal.
	Projects::RegisterFileType();

	mLogo = Texture::Create(kEngineIconFile, TextureFilter::Linear);
	mBanner = Texture::Create("Images/lion-engine-banner.png", TextureFilter::Linear);

	Refresh();
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

void ProjectManagerLayer::Refresh()
{
	mEntries = Projects::LoadAll();

	// A selection that outlived its entry selects nothing.
	if (!Selected())
		mSelected.clear();
}

const Projects::Entry* ProjectManagerLayer::Selected() const
{
	for (const Projects::Entry& entry : mEntries)
		if (entry.path == mSelected)
			return &entry;

	return nullptr;
}

void ProjectManagerLayer::DrawManager()
{
	// One window filling the frame: the manager is this page, not a panel arrangement.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("##project_manager", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::PopStyleVar();

	const float32 width = ImGui::GetWindowWidth();

	DrawCaption(width);
	DrawBanner(width);

	// --- The page: toolbar, then the list beside its actions, then the version in the corner.
	ImGui::SetCursorPos(ImVec2(kPadding, kCaptionHeight + kBannerHeight + kPadding * 0.75f));
	ImGui::BeginGroup();

	DrawToolbar();
	ImGui::Spacing();

	const float32 footer = ImGui::GetTextLineHeightWithSpacing() + kPadding * 0.75f;
	const float32 bodyHeight = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - footer;
	const float32 listWidth = width - kPadding * 2.0f - kActionsWidth - 8.0f;

	ImGui::BeginChild("##projects", ImVec2(listWidth, bodyHeight), ImGuiChildFlags_Borders);
	DrawProjectList();
	ImGui::EndChild();

	ImGui::SameLine(0.0f, 8.0f);

	ImGui::BeginChild("##actions", ImVec2(kActionsWidth, bodyHeight), ImGuiChildFlags_Borders);
	DrawActionsPanel();
	ImGui::EndChild();

	ImGui::EndGroup();

	ImGui::SetCursorPos(ImVec2(kPadding, ImGui::GetWindowHeight() - footer + kPadding * 0.25f));
	ImGui::TextDisabled("Lion Engine  %s", kVersion);

	DrawPopups();

	// What is left of the caption is what the window is dragged by; what was drawn in it is not.
	const bool overBar = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
	Window::SetTitleBarBlocked(!overBar || ImGui::IsAnyItemHovered());

	ImGui::End();
}

void ProjectManagerLayer::DrawCaption(float32 width)
{
	const ImVec2 barMin = ImGui::GetWindowPos();

	ImDrawList* draw = ImGui::GetWindowDrawList();
	draw->AddRectFilled(barMin, ImVec2(barMin.x + width, barMin.y + kCaptionHeight),
		ImGui::GetColorU32(ImGuiCol_MenuBarBg));

	// The mark at the left, upright — bottom-up, as OpenGL loaded it.
	constexpr float32 kMark = 24.0f;

	if (mLogo)
		draw->AddImage(static_cast<ImTextureID>(mLogo->GetNativeHandle()),
			ImVec2(barMin.x + 12.0f, barMin.y + (kCaptionHeight - kMark) * 0.5f),
			ImVec2(barMin.x + 12.0f + kMark, barMin.y + (kCaptionHeight + kMark) * 0.5f),
			kFlipTop, kFlipBottom);

	// The window's name in the middle of the strip, where a window says what it is.
	constexpr const char8* kTitle = "Lion Engine - Project Manager";

	ImGui::PushFont(EditorGui::GetBoldFont());
	const float32 titleWidth = ImGui::CalcTextSize(kTitle).x;
	ImGui::SetCursorPos(ImVec2((width - titleWidth) * 0.5f, (kCaptionHeight - ImGui::GetTextLineHeight()) * 0.5f));
	ImGui::TextUnformatted(kTitle);
	ImGui::PopFont();

	// Minimise and close, the way the editor draws its own — there is no maximise, because there is no
	// bigger size this window has.
	constexpr float32 kArm = 5.0f;

	for (int32 kind = 0; kind < 2; ++kind)
	{
		const float32 left = barMin.x + width - kWindowButton * (2 - kind);

		ImGui::PushID(kind);
		ImGui::SetCursorScreenPos(ImVec2(left, barMin.y));

		const bool pressed = ImGui::InvisibleButton("##caption_button", ImVec2(kWindowButton, kCaptionHeight));
		const bool hovered = ImGui::IsItemHovered();

		ImGui::PopID();

		// Minimise green, close red — the editor's traffic light, minus the amber it has no maximise for.
		const ImU32 fill = kind == 0 ? IM_COL32(46, 160, 67, 255) : IM_COL32(196, 43, 28, 255);

		if (hovered)
			draw->AddRectFilled(ImVec2(left, barMin.y), ImVec2(left + kWindowButton, barMin.y + kCaptionHeight), fill);

		const ImVec2 center(left + kWindowButton * 0.5f, barMin.y + kCaptionHeight * 0.5f);
		const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);

		if (kind == 0)
		{
			draw->AddLine(ImVec2(center.x - kArm, center.y), ImVec2(center.x + kArm, center.y), color, 1.0f);
		}
		else
		{
			draw->AddLine(ImVec2(center.x - kArm, center.y - kArm), ImVec2(center.x + kArm, center.y + kArm), color, 1.2f);
			draw->AddLine(ImVec2(center.x - kArm, center.y + kArm), ImVec2(center.x + kArm, center.y - kArm), color, 1.2f);
		}

		if (pressed)
			kind == 0 ? Window::Minimize() : Window::RequestClose();
	}
}

void ProjectManagerLayer::DrawBanner(float32 width)
{
	const ImVec2 windowMin = ImGui::GetWindowPos();
	const ImVec2 bannerMin(windowMin.x, windowMin.y + kCaptionHeight);
	const ImVec2 bannerMax(windowMin.x + width, bannerMin.y + kBannerHeight);

	ImDrawList* draw = ImGui::GetWindowDrawList();

	if (!mBanner)
	{
		draw->AddRectFilled(bannerMin, bannerMax, IM_COL32(20, 20, 22, 255));
		return;
	}

	// Stretched to the width and cropped to the strip, like a cover: the middle band of the image is the
	// part that carries the mark. The V runs backwards because OpenGL keeps its images bottom-up.
	const Size size = mBanner->GetSize();
	const float32 scale = width / size.width;
	const float32 visible = std::min(1.0f, kBannerHeight / (scale * size.height));
	const float32 crop = (1.0f - visible) * 0.5f;

	draw->AddImage(static_cast<ImTextureID>(mBanner->GetNativeHandle()), bannerMin, bannerMax,
		ImVec2(0.0f, 1.0f - crop), ImVec2(1.0f, crop));
}

void ProjectManagerLayer::DrawToolbar()
{
	if (ImGui::Button(ICON_MDI_PLUS "  Create", ImVec2(110.0f, 0.0f)))
	{
		mOpenCreatePopup = true;
		mCreateError.clear();
		mName[0] = '\0';
		mLocation[0] = '\0';
	}

	ImGui::SameLine();

	// Import: a project already on disk, brought onto the list — Godot's word for it.
	if (ImGui::Button(ICON_MDI_FOLDER_OPEN "  Import", ImVec2(110.0f, 0.0f)))
	{
		const std::string picked = FileDialog::OpenFolder(std::string());

		if (!picked.empty())
		{
			if (Projects::IsProjectFolder(picked))
			{
				Projects::Remember(picked);
				Refresh();
				mSelected = std::filesystem::path(picked).generic_string();
			}
			else
			{
				mCreateError = "'" + picked + "' is not a Lion project (no Assets folder).";
			}
		}
	}

	// The filter takes the room between the buttons and the sort.
	constexpr float32 kSortWidth = 150.0f;
	const float32 sortLabel = ImGui::CalcTextSize("Sort:").x;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - kActionsWidth + (kActionsWidth - kSortWidth - sortLabel - 24.0f));
	ImGui::InputTextWithHint("##filter", ICON_MDI_MAGNIFY "  Filter projects", mFilter, sizeof(mFilter));

	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Sort:");
	ImGui::SameLine();

	constexpr const char8* kSortNames[] = { "Last Edited", "Name", "Path" };
	int32 sort = static_cast<int32>(mSort);

	ImGui::SetNextItemWidth(kSortWidth);

	if (ImGui::Combo("##sort", &sort, kSortNames, 3))
		mSort = static_cast<Sort>(sort);

	if (!mCreateError.empty() && !mOpenCreatePopup)
	{
		ImGui::TextColored(ImVec4(0.90f, 0.35f, 0.35f, 1.0f), "%s", mCreateError.c_str());
	}
}

void ProjectManagerLayer::DrawProjectList()
{
	// Sorted fresh each frame from the loaded entries: favourites first, then the chosen order. The list
	// is a handful of projects, not a scene graph — clarity beats caching here.
	std::vector<const Projects::Entry*> ordered;
	ordered.reserve(mEntries.size());

	const std::string filter = [this]
	{
		std::string text = mFilter;
		std::transform(text.begin(), text.end(), text.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return text;
	}();

	for (const Projects::Entry& entry : mEntries)
	{
		if (!filter.empty())
		{
			std::string name = entry.name;
			std::transform(name.begin(), name.end(), name.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			if (name.find(filter) == std::string::npos)
				continue;
		}

		ordered.push_back(&entry);
	}

	const Sort sort = mSort;

	std::stable_sort(ordered.begin(), ordered.end(),
		[sort](const Projects::Entry* a, const Projects::Entry* b)
		{
			if (a->favorite != b->favorite)
				return a->favorite;

			switch (sort)
			{
				case Sort::Name: return a->name < b->name;
				case Sort::Path: return a->path < b->path;
				default:         return a->editedAt > b->editedAt;
			}
		});

	if (ordered.empty())
	{
		ImGui::SetCursorPos(ImVec2(kPadding, kPadding));
		ImGui::TextDisabled(mEntries.empty() ? "No projects yet - create one above." : "Nothing matches the filter.");
		return;
	}

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	const ImU32 dimColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	const float32 lineHeight = ImGui::GetTextLineHeight();
	const ImVec4 accent = EditorGui::GetAccent();
	const ImU32 accentColor = ImGui::GetColorU32(accent);

	std::string openRequest;

	for (const Projects::Entry* entry : ordered)
	{
		ImGui::PushID(entry->path.c_str());

		const ImVec2 rowMin = ImGui::GetCursorScreenPos();
		const float32 rowWidth = ImGui::GetContentRegionAvail().x;

		// The star first, its own button, so favouriting does not select or open.
		constexpr float32 kStarLane = 34.0f;

		ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMin.y));

		if (ImGui::InvisibleButton("##favorite", ImVec2(kStarLane, kRowHeight)))
		{
			Projects::SetFavorite(entry->path, !entry->favorite);
			Refresh();
			ImGui::PopID();
			return;   // The entries were just replaced; finish this frame's list here.
		}

		const bool starHovered = ImGui::IsItemHovered();

		if (starHovered)
			ImGui::SetTooltip("%s", entry->favorite ? "Unfavorite" : "Favorite");

		// Then the row itself: one selectable the height of both its lines. A click selects, a double
		// click opens — a missing project takes neither.
		ImGui::SetCursorScreenPos(ImVec2(rowMin.x + kStarLane, rowMin.y));

		const bool isSelected = (entry->path == mSelected);

		if (ImGui::Selectable("##row", isSelected, ImGuiSelectableFlags_AllowDoubleClick,
			ImVec2(rowWidth - kStarLane, kRowHeight)))
		{
			if (!entry->missing)
			{
				mSelected = entry->path;

				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					openRequest = entry->path;
			}
		}

		DrawStar(draw, ImVec2(rowMin.x + kStarLane * 0.5f, rowMin.y + kRowHeight * 0.5f), 7.0f,
			entry->favorite, entry->favorite ? accentColor : (starHovered ? textColor : dimColor));

		// The engine's mark on every project, upright, in a plate of its own.
		constexpr float32 kMark = 36.0f;
		const ImVec2 markMin(rowMin.x + kStarLane + 6.0f, rowMin.y + (kRowHeight - kMark) * 0.5f);

		draw->AddRectFilled(markMin, ImVec2(markMin.x + kMark, markMin.y + kMark), IM_COL32(28, 28, 30, 255), 6.0f);

		if (mLogo)
			draw->AddImage(static_cast<ImTextureID>(mLogo->GetNativeHandle()),
				markMin, ImVec2(markMin.x + kMark, markMin.y + kMark), kFlipTop, kFlipBottom);

		// The name over the path; a project that left the disk says so instead of a date.
		const float32 textLeft = markMin.x + kMark + 12.0f;
		const float32 topLine = rowMin.y + (kRowHeight - lineHeight * 2.0f - 4.0f) * 0.5f;

		ImFont* bold = EditorGui::GetBoldFont();
		draw->AddText(bold, bold ? bold->FontSize : lineHeight, ImVec2(textLeft, topLine),
			entry->missing ? dimColor : textColor, entry->name.c_str());
		draw->AddText(ImVec2(textLeft, topLine + lineHeight + 4.0f), dimColor, entry->path.c_str());

		// The engine version over the last-edited stamp, kept to the right the way Godot keeps them.
		const std::string right = entry->missing ? std::string("(missing)") : entry->version;
		const std::string below = entry->missing ? std::string() : entry->edited;

		if (!right.empty())
		{
			const float32 rightWidth = ImGui::CalcTextSize(right.c_str()).x;
			draw->AddText(ImVec2(rowMin.x + rowWidth - rightWidth - kPadding, topLine),
				entry->missing ? dimColor : textColor, right.c_str());
		}

		if (!below.empty())
		{
			const float32 belowWidth = ImGui::CalcTextSize(below.c_str()).x;
			draw->AddText(ImVec2(rowMin.x + rowWidth - belowWidth - kPadding, topLine + lineHeight + 4.0f),
				dimColor, below.c_str());
		}

		ImGui::PopID();
	}

	if (!openRequest.empty())
		OpenInEditor(openRequest);
}

void ProjectManagerLayer::DrawActionsPanel()
{
	const Projects::Entry* selected = Selected();
	const ImVec2 button(ImGui::GetContentRegionAvail().x, 0.0f);

	ImGui::PushFont(EditorGui::GetBoldFont());
	ImGui::TextUnformatted("Actions");
	ImGui::PopFont();
	ImGui::Spacing();

	ImGui::BeginDisabled(!selected || selected->missing);

	if (ImGui::Button("Open", button) && selected)
		OpenInEditor(selected->path);

	ImGui::EndDisabled();

	// The built-in keeps its name and its place on the list: it is the engine's, not the machine's.
	ImGui::BeginDisabled(!selected || selected->missing || selected->builtIn);

	if (ImGui::Button("Rename", button) && selected)
	{
		mOpenRenamePopup = true;
		mRenameError.clear();

		const size_t copied = selected->name.copy(mRenameBuffer, sizeof(mRenameBuffer) - 1);
		mRenameBuffer[copied] = '\0';
	}

	ImGui::EndDisabled();

	ImGui::BeginDisabled(!selected || selected->missing);

	if (ImGui::Button("Duplicate", button) && selected)
	{
		std::string error;
		const std::filesystem::path copy = Projects::Duplicate(selected->path, error);

		if (!copy.empty())
		{
			Refresh();
			mSelected = copy.generic_string();
		}
	}

	ImGui::EndDisabled();

	ImGui::BeginDisabled(!selected || selected->builtIn);

	if (ImGui::Button("Remove", button) && selected)
		mOpenRemovePopup = true;

	ImGui::EndDisabled();

	const bool anyMissing = std::any_of(mEntries.begin(), mEntries.end(),
		[](const Projects::Entry& entry) { return entry.missing; });

	ImGui::BeginDisabled(!anyMissing);

	if (ImGui::Button("Remove Missing", button))
	{
		Projects::RemoveMissing();
		Refresh();
	}

	ImGui::EndDisabled();

	// Donate keeps the door open at the bottom, the way Godot keeps it. It goes nowhere yet.
	ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - 8.0f);
	ImGui::Button("Donate", button);
}

void ProjectManagerLayer::DrawPopups()
{
	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();

	// --- Create.
	if (mOpenCreatePopup)
	{
		mOpenCreatePopup = false;
		ImGui::OpenPopup("Create Project");
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float32 browseWidth = ImGui::CalcTextSize(ICON_MDI_FOLDER_OPEN).x + style.FramePadding.x * 2.0f;

		// The field the popup exists for starts with the keyboard in it.
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();

		ImGui::SetNextItemWidth(360.0f);
		ImGui::InputTextWithHint("##new_name", "Project name", mName, sizeof(mName));

		ImGui::SetNextItemWidth(360.0f - browseWidth - style.ItemInnerSpacing.x);
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

		if (!mCreateError.empty())
			ImGui::TextColored(ImVec4(0.90f, 0.35f, 0.35f, 1.0f), "%s", mCreateError.c_str());

		ImGui::Spacing();
		ImGui::BeginDisabled(mName[0] == '\0' || mLocation[0] == '\0');

		if (ImGui::Button("Create & Open", ImVec2(130.0f, 0.0f)))
		{
			mCreateError.clear();
			const std::filesystem::path folder = Projects::CreateOnDisk(mName, mLocation, mCreateError);

			if (!folder.empty())
			{
				ImGui::CloseCurrentPopup();
				OpenInEditor(folder.generic_string());
			}
		}

		ImGui::EndDisabled();
		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	// --- Rename.
	if (mOpenRenamePopup)
	{
		mOpenRenamePopup = false;
		ImGui::OpenPopup("Rename Project");
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Rename Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// The field the popup exists for starts with the keyboard in it, the old name selected to be
		// typed over.
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();

		ImGui::SetNextItemWidth(320.0f);
		const bool submitted = ImGui::InputText("##rename", mRenameBuffer, sizeof(mRenameBuffer),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

		if (!mRenameError.empty())
			ImGui::TextColored(ImVec4(0.90f, 0.35f, 0.35f, 1.0f), "%s", mRenameError.c_str());

		ImGui::Spacing();
		ImGui::BeginDisabled(mRenameBuffer[0] == '\0');

		if ((ImGui::Button("Rename", ImVec2(96.0f, 0.0f)) || submitted) && Selected())
		{
			mRenameError.clear();

			if (Projects::Rename(Selected()->path, mRenameBuffer, mRenameError))
			{
				Refresh();
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndDisabled();
		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	// --- Remove.
	if (mOpenRemovePopup)
	{
		mOpenRemovePopup = false;
		ImGui::OpenPopup("Remove Project");
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Remove Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const Projects::Entry* selected = Selected();

		ImGui::Text("Remove '%s' from the list?", selected ? selected->name.c_str() : "");
		ImGui::TextDisabled("The files stay where they are; only the list forgets them.");
		ImGui::Spacing();

		if (ImGui::Button("Remove", ImVec2(96.0f, 0.0f)) && selected)
		{
			Projects::RemoveRecent(selected->path);
			mSelected.clear();
			Refresh();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}

void ProjectManagerLayer::OpenInEditor(const std::string& project)
{
	Projects::Remember(project);

	Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[ProjectManager] Opening '{}'.", Projects::DisplayName(project)));

	// The editor is its own process on this one project; the manager's job ends where the editor's begins.
	LaunchSelf("--project \"" + project + "\"");
	Window::RequestClose();
}
