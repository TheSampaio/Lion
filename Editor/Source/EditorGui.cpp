#include "EditorPch.h"
#include "EditorGui.h"
#include "Projects.h"

#include <filesystem>

#include <Lion/Lion.h>
#include <Lion/Core/Filesystem.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <IconsMaterialDesignIcons.h>

using namespace Lion;

// The bold cut, and the icon font as a font of its own. Both are kept because ImGui hands them back as
// nothing more than a pointer into its font atlas.
static ImFont* sBoldFont = nullptr;
static ImFont* sIconFont = nullptr;

ImVec4 EditorGui::GetAccent()
{
	// #D96D1F — the orange of the engine's mark, a shade deeper than the logo's so white text and white
	// icons stay legible on top of it. The colour is written once: a second copy of it is a second thing to
	// remember when the brand moves.
	return ImVec4(0.851f, 0.427f, 0.122f, 1.0f);
}

// One size for the text and the inline icons that sit in a run of it — a menu item, a tab, a button label.
constexpr float32 kFontSize = 18.0f;

// The icons drawn by hand (the docks, the Hierarchy rows, the console) are baked once at the largest size
// any of them asks for, and scaled down where a caller wants less. Down is sharp; up is not, which is why
// the whole set is baked big rather than at each size it is used.
constexpr float32 kIconAtlasSize = 32.0f;

ImFont* EditorGui::GetBoldFont() { return sBoldFont; }
ImFont* EditorGui::GetIconFont() { return sIconFont; }
float32 EditorGui::GetIconAtlasSize() { return kIconAtlasSize; }

// The icon font, twice over: merged into the text so an icon can sit inline with a label, and standalone at
// atlas resolution so a hand-drawn icon is sharp at 20, 24 or 32 pixels. One .ttf, two ImFonts.
void LoadIconFont()
{
	const std::filesystem::path font = std::filesystem::path(ResourceRootDirectory()) / "Fonts" / FONT_ICON_FILE_NAME_MDI;

	if (!std::filesystem::exists(font))
	{
		Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[Editor] Icon font not found: '{}'.", font.string()));
		return;
	}

	// Only the glyphs the editor actually draws, not the whole 7,400-icon set. The font is baked twice — once
	// merged at 16px, once standalone at 32px — and the full range at both sizes made an atlas texture too
	// large to upload, which is why the editor opened as a blank white sheet: no font texture, nothing to
	// draw the UI with. A few dozen glyphs at two sizes is nothing.
	//
	// A glyph left off this list comes out blank, so an icon added to the editor is added here too — the one
	// upkeep the subset costs.
	static const char8* kUsedIcons =
		ICON_MDI_ALERT ICON_MDI_ALERT_CIRCLE ICON_MDI_ARROW_ALL ICON_MDI_ARROW_EXPAND_ALL ICON_MDI_ARROW_UP
		ICON_MDI_AXIS_ARROW ICON_MDI_CHART_BAR ICON_MDI_CODE_TAGS ICON_MDI_COG ICON_MDI_CONSOLE
		ICON_MDI_CONTENT_SAVE_ALL_OUTLINE ICON_MDI_CONTENT_SAVE_OUTLINE ICON_MDI_CUBE_OUTLINE
		ICON_MDI_CURSOR_DEFAULT ICON_MDI_DELETE_OUTLINE ICON_MDI_EXIT_TO_APP ICON_MDI_EYE ICON_MDI_EYE_OFF
		ICON_MDI_FILE_DOCUMENT_OUTLINE ICON_MDI_FILE_PLUS_OUTLINE ICON_MDI_FILE_TREE ICON_MDI_FOLDER
		ICON_MDI_FOLDER_MULTIPLE ICON_MDI_FOLDER_OPEN ICON_MDI_FOLDER_OPEN_OUTLINE ICON_MDI_HAMMER_WRENCH
		ICON_MDI_IMAGE ICON_MDI_IMAGE_OUTLINE ICON_MDI_INFORMATION ICON_MDI_KEYBOARD ICON_MDI_LOCK
		ICON_MDI_LOCK_OPEN_VARIANT ICON_MDI_MAGNIFY ICON_MDI_MONITOR ICON_MDI_PALETTE ICON_MDI_PAUSE
		ICON_MDI_PLAY ICON_MDI_PLUS ICON_MDI_PUZZLE ICON_MDI_REDO ICON_MDI_RELOAD ICON_MDI_RESTORE
		ICON_MDI_ROTATE_RIGHT ICON_MDI_SHAPE ICON_MDI_STEP_FORWARD ICON_MDI_STOP ICON_MDI_TUNE ICON_MDI_UNDO
		ICON_MDI_VECTOR_CIRCLE_VARIANT ICON_MDI_VECTOR_SQUARE ICON_MDI_VIEW_DASHBOARD_OUTLINE ICON_MDI_WEIGHT;

	// ImGui keeps the pointer, not the array, so the built range outlives this call.
	static ImVector<ImWchar> range;
	ImFontGlyphRangesBuilder builder;
	builder.AddText(kUsedIcons);
	builder.BuildRanges(&range);

	// Merged into the text font at 16px — the size Unity, Godot and Unreal set an inline icon to. An icon
	// fills its em square where a letter leaves room in theirs, so 16 against 18px text is what makes the two
	// weigh the same in a menu, a tab or a component header. It sits on the baseline, nudged to level its ink
	// box with the words'.
	ImFontConfig merged;
	merged.MergeMode = true;
	merged.PixelSnapH = true;
	merged.GlyphMinAdvanceX = 16.0f;
	merged.GlyphOffset.y = 1.0f;

	ImGui::GetIO().Fonts->AddFontFromFileTTF(font.string().c_str(), 16.0f, &merged, range.Data);

	// And once more on its own, at atlas size, for the icons the editor positions itself.
	ImFontConfig standalone;
	standalone.PixelSnapH = true;

	sIconFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(font.string().c_str(), kIconAtlasSize, &standalone, range.Data);
}

static void SetDarkTheme()
{
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();

	// Layout: generous, consistent spacing and gentle rounding for a modern, calm look. Every metric
	// sits on a 2px grid, so nested paddings stack into whole pixels instead of drifting half a one.
	style.WindowPadding     = ImVec2(8.0f, 8.0f);
	style.FramePadding      = ImVec2(8.0f, 4.0f);
	style.CellPadding       = ImVec2(6.0f, 4.0f);
	style.ItemSpacing       = ImVec2(8.0f, 6.0f);
	style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
	style.IndentSpacing     = 20.0f;
	style.ScrollbarSize     = 12.0f;
	style.GrabMinSize       = 8.0f;

	style.WindowBorderSize  = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.PopupBorderSize   = 1.0f;
	style.TabBarBorderSize  = 1.0f;

	style.WindowRounding    = 6.0f;
	style.ChildRounding     = 6.0f;
	style.FrameRounding     = 4.0f;
	style.PopupRounding     = 6.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding      = 4.0f;
	style.TabRounding       = 4.0f;

	style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.SeparatorTextBorderSize  = 1.0f;

	// Palette: neutral graphite surfaces under the engine's own orange, which is the colour of the mark on
	// the window and the icon in Explorer. A blue accent is what every dark theme ships with; this one is the
	// Lion's, and an editor that looks like itself is worth the two lines it takes.
	const ImVec4 accent      = EditorGui::GetAccent();
	const ImVec4 accentHover = ImVec4(0.96f, 0.56f, 0.24f, 1.00f);
	const ImVec4 accentDim   = ImVec4(accent.x, accent.y, accent.z, 0.45f);

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text]                  = ImVec4(0.88f, 0.89f, 0.91f, 1.00f);
	colors[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.46f, 0.49f, 1.00f);
	colors[ImGuiCol_WindowBg]              = ImVec4(0.115f, 0.120f, 0.130f, 1.00f);
	colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]               = ImVec4(0.145f, 0.150f, 0.165f, 1.00f);
	colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.45f);
	colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	colors[ImGuiCol_FrameBg]               = ImVec4(0.075f, 0.078f, 0.086f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.115f, 0.120f, 0.135f, 1.00f);
	colors[ImGuiCol_FrameBgActive]         = ImVec4(0.150f, 0.155f, 0.175f, 1.00f);

	colors[ImGuiCol_TitleBg]               = ImVec4(0.09f, 0.095f, 0.105f, 1.00f);
	colors[ImGuiCol_TitleBgActive]         = ImVec4(0.11f, 0.115f, 0.130f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.09f, 0.095f, 0.105f, 1.00f);
	colors[ImGuiCol_MenuBarBg]             = ImVec4(0.105f, 0.110f, 0.120f, 1.00f);

	colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.29f, 0.32f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.36f, 0.37f, 0.40f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.42f, 0.43f, 0.46f, 1.00f);

	colors[ImGuiCol_CheckMark]             = accent;
	colors[ImGuiCol_SliderGrab]            = accent;
	colors[ImGuiCol_SliderGrabActive]      = accentHover;

	colors[ImGuiCol_Button]                = ImVec4(0.19f, 0.20f, 0.23f, 1.00f);
	colors[ImGuiCol_ButtonHovered]         = ImVec4(0.25f, 0.26f, 0.30f, 1.00f);
	colors[ImGuiCol_ButtonActive]          = ImVec4(0.29f, 0.30f, 0.34f, 1.00f);

	colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered]         = ImVec4(0.23f, 0.24f, 0.28f, 1.00f);
	colors[ImGuiCol_HeaderActive]          = ImVec4(0.27f, 0.28f, 0.32f, 1.00f);

	colors[ImGuiCol_Separator]             = ImVec4(0.00f, 0.00f, 0.00f, 0.45f);
	colors[ImGuiCol_SeparatorHovered]      = accentHover;
	colors[ImGuiCol_SeparatorActive]       = accentHover;

	colors[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.29f, 0.32f, 0.60f);
	colors[ImGuiCol_ResizeGripHovered]     = accentDim;
	colors[ImGuiCol_ResizeGripActive]      = accent;

	colors[ImGuiCol_Tab]                   = ImVec4(0.115f, 0.120f, 0.135f, 1.00f);
	colors[ImGuiCol_TabHovered]            = ImVec4(0.24f, 0.26f, 0.32f, 1.00f);
	colors[ImGuiCol_TabSelected]           = ImVec4(0.17f, 0.20f, 0.27f, 1.00f);
	colors[ImGuiCol_TabSelectedOverline]   = accent;
	colors[ImGuiCol_TabDimmed]             = ImVec4(0.105f, 0.110f, 0.120f, 1.00f);
	colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.135f, 0.145f, 0.165f, 1.00f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(accent.x, accent.y, accent.z, 0.30f);

	colors[ImGuiCol_DockingPreview]        = accentDim;
	colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.09f, 0.095f, 0.105f, 1.00f);

	colors[ImGuiCol_TextSelectedBg]        = accentDim;
	colors[ImGuiCol_NavHighlight]          = accent;
	colors[ImGuiCol_DragDropTarget]        = accentHover;

	colors[ImGuiCol_PlotHistogram]         = accent;
	colors[ImGuiCol_PlotHistogramHovered]  = accentHover;
}

void EditorGui::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// The editor's persisted state lives in the one Data folder Projects.h decides on — beside the
	// executable, or in the user's app data when that folder refuses writes. ImGui only holds a pointer
	// to the filename, so it lives in a static string, and it will not create the folder itself. The
	// layouts folder and the shortcuts config sit alongside it.
	const std::filesystem::path dataDirectory = Projects::EditorDataDirectory();

	std::error_code error;
	std::filesystem::create_directories(dataDirectory, error);

	// Named for the layout it stores, not for the editor: a panel is identified by its window's name, the
	// names now carry an icon, and a file written before that describes windows that no longer exist. A new
	// file starts the session on the default layout instead of on six panels floating over each other.
	static const std::string iniPath = (dataDirectory / "lion-layout.ini").string();
	io.IniFilename = iniPath.c_str();

	// The glyphs the text font carries. ImGui's default is Basic Latin and nothing else, so a dash, an
	// ellipsis or an accented name comes out as a hollow box — the em-dash that did. General Punctuation
	// comes along, and the Latin block a Portuguese name needs.
	static const ImWchar textRange[] = {
		0x0020, 0x00FF,   // Basic Latin + Latin-1 Supplement (accented letters).
		0x2010, 0x205E,   // General Punctuation (dashes, quotes, the ellipsis).
		0,
	};

	// Prefer Segoe UI (much more legible than ImGui's built-in font); fall back to the default. The bold cut
	// comes along for the few places that have to weigh more than what is around them; without it, a caller
	// asking for bold gets a null font and ImGui draws its own.
	if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf"))
		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", kFontSize, nullptr, textRange);

	LoadIconFont();

	if (std::filesystem::exists("C:/Windows/Fonts/segoeuib.ttf"))
		sBoldFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeuib.ttf", kFontSize, nullptr, textRange);

	SetDarkTheme();

	auto* window = static_cast<GLFWwindow*>(Window::GetNativeHandle());
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void EditorGui::Shutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void EditorGui::BeginFrame()
{
	// The window draws its own caption, so the edges it is resized by are inside the area the GUI believes is
	// entirely its own — and the GUI sets a cursor there every frame, over the double-headed one Windows put
	// there on the way in. Whoever writes last wins, and the GUI writes last: the edge stops looking like an
	// edge, and a border you cannot see is a border you cannot grab. Over an edge, the GUI does not get a say.
	ImGuiIO& io = ImGui::GetIO();

	if (Window::IsPointerOnResizeBorder())
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	else
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void EditorGui::EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
