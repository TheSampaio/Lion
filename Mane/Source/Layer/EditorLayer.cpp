#include "EditorLayer.h"

#include "../EditorGui.h"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <Lion/Core/Filesystem.h>
#include <Lion/Logic/ComponentRegistry.h>

#include <imgui/imgui_internal.h> // DockBuilder API for the default layout.

using namespace Lion;

void EditorLayer::OnAttach()
{
	Window::SetSize(1280, 720);
	Window::SetTitle("Lion Editor");
	Window::SetBackgroundColor(0.10f, 0.10f, 0.11f);
	Window::SetResizable(true);
	Window::SetMaximized(true);
}

void EditorLayer::OnCreate()
{
	EditorGui::Init();
	InitShortcuts();

	LoadGameModule();

	mCamera = MakeReference<CameraOrthographic>();
	mScene = MakeReference<Scene>();

	FramebufferSpecification spec;
	spec.width = 1280;
	spec.height = 720;
	spec.hasEntityId = true;   // Secondary attachment for pixel-perfect viewport picking.
	mFramebuffer = Framebuffer::Create(spec);

	CreateDemoScene();
}

void EditorLayer::OnUpdate()
{
	PollGameBuild();

	// Advance the scene simulation (physics + entity scripts) while playing and not paused — or for
	// exactly one frame when a step was requested, which is what makes a paused run inspectable.
	if (mPlaying && (!mPaused || mStepFrame))
	{
		mScene->OnUpdate();
		mStepFrame = false;
	}
}

void EditorLayer::StepOneFrame()
{
	if (!mPlaying)
		return;

	// Stepping only makes sense against a halted simulation, so a running one is paused first: the
	// frame this schedules is then the only one that advances.
	mPaused = true;
	mStepFrame = true;
}

void EditorLayer::OnDetach()
{
	EditorGui::Shutdown();
}

int EditorLayer::SelectedEntityIndex() const
{
	if (!mSelectedEntity)
		return -1;

	int index = 0;
	for (const auto& entity : mScene->GetEntities())
	{
		if (entity == mSelectedEntity)
			return index;

		index++;
	}

	return -1;
}

void EditorLayer::SelectEntityByIndex(int index)
{
	mSelectedEntity = nullptr;
	mRenamingEntity = nullptr;

	if (index < 0)
		return;

	int current = 0;
	for (const auto& entity : mScene->GetEntities())
	{
		if (current == index)
		{
			mSelectedEntity = entity;
			return;
		}

		current++;
	}
}

void EditorLayer::StartPlay()
{
	if (mPlaying)
	{
		mPaused = false;  // Play acts as "resume" while paused.
		return;
	}

	// Save the edited scene, then rebuild it so every component runs OnAwake again (which creates
	// the Box2D bodies/shapes needed for the simulation). The rebuild preserves entity order, so the
	// selection is restored by index.
	const int selected = SelectedEntityIndex();

	mPlaySnapshot = SceneSerializer::SerializeToString(mScene);
	SceneSerializer::DeserializeFromString(mScene, mPlaySnapshot);
	SelectEntityByIndex(selected);

	mPlaying = true;
	mPaused = false;
	Log::Console(LogLevel::Information, "[Editor] Play mode started.");
}

void EditorLayer::TogglePause()
{
	if (!mPlaying)
		return;

	mPaused = !mPaused;
	Log::Console(LogLevel::Information, mPaused ? "[Editor] Play mode paused." : "[Editor] Play mode resumed.");
}

void EditorLayer::StopPlay()
{
	if (!mPlaying)
		return;

	// Restore the scene to exactly the edited state captured when Play started.
	const int selected = SelectedEntityIndex();

	SceneSerializer::DeserializeFromString(mScene, mPlaySnapshot);
	SelectEntityByIndex(selected);

	mPlaying = false;
	mPaused = false;
	Log::Console(LogLevel::Information, "[Editor] Play mode stopped.");
}

void EditorLayer::CreateDemoScene()
{
	// Background.
	auto background = MakeReference<Entity>();
	background->SetName("Background");
	background->GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Back));
	background->AddComponent<SpriteRenderer>("Sprite/Brickout/background.jpg");
	mScene->Add(background);

	// A row of bricks.
	for (int32 i = 0; i < 5; i++)
	{
		auto brick = MakeReference<Entity>();
		brick->SetName("Brick " + std::to_string(i + 1));
		brick->GetTransform()->SetPosition(Vector(-160.0f + i * 80.0f, 60.0f, Depth::Middle));
		brick->AddComponent<SpriteRenderer>("Sprite/Brickout/tile-" + std::to_string(i + 1) + ".png");
		mScene->Add(brick);
	}

	// Ball (with physics, so pressing Play drops it under gravity).
	auto ball = MakeReference<Entity>();
	ball->SetName("Ball");
	ball->GetTransform()->SetPosition(Vector(0.0f, 120.0f, Depth::Middle));
	ball->AddComponent<SpriteRenderer>("Sprite/Brickout/ball.png");
	ball->AddComponent<RigidBody2D>(BodyType::Dynamic, false);
	ball->AddComponent<CircleCollider2D>(16.0f);
	mScene->Add(ball);
}

void EditorLayer::OnRender()
{
	// The scene is rendered into the framebuffer first, then the editor UI is drawn on top of
	// the window with the framebuffer shown inside the Viewport panel.
	RenderScene();

	EditorGui::BeginFrame();
	ImGuizmo::BeginFrame();

	DrawUI();

	EditorGui::EndFrame();
}

void EditorLayer::RenderScene()
{
	// Match the framebuffer/camera to the viewport panel size measured last frame.
	const FramebufferSpecification& spec = mFramebuffer->GetSpecification();
	const uint32 targetWidth = static_cast<uint32>(mViewportSize.x);
	const uint32 targetHeight = static_cast<uint32>(mViewportSize.y);

	if (targetWidth > 0 && targetHeight > 0 && (targetWidth != spec.width || targetHeight != spec.height))
	{
		mFramebuffer->Resize(targetWidth, targetHeight);
		mCamera->OnResize(mViewportSize.x, mViewportSize.y);
	}

	// Render the scene into the framebuffer instead of the window.
	mFramebuffer->Bind();
	Renderer::Clear(0.12f, 0.12f, 0.15f, 1.0f);
	mFramebuffer->ClearEntityId(-1);  // Empty pixels map to "no entity".

	Renderer::RenderBegin(mCamera);
	mScene->OnRender();
	Renderer::RenderEnd();

	mFramebuffer->Unbind();
}

void EditorLayer::DrawUI()
{
	ApplyPendingLayout();
	DrawMenuBar();
	HandleShortcuts();

	// Fullscreen, borderless host window that holds the dockspace (below the main menu bar).
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	const ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("LionEditorDockHost", nullptr, hostFlags);
	ImGui::PopStyleVar(3);

	// Kept for the next frame's ApplyPendingLayout: GetID reads the current window's ID stack, and by
	// then no window is being submitted.
	mDockspaceId = ImGui::GetID("LionEditorDockspace");

	// With no layout on disk, fall back to the default one — but request it rather than building it
	// here. On the first frame the viewport's work area does not yet exclude the main menu bar (ImGui
	// only applies that inset on the following frame), so a layout built now would be sized against a
	// taller area and then rescaled, landing every panel a few pixels off what "Default" gives later.
	// Deferring puts the boot through the exact same path as the menu, so the two cannot disagree.
	if (!mLayoutInitialized)
	{
		mLayoutInitialized = true;

		if (ImGui::DockBuilderGetNode(mDockspaceId) == nullptr)
			mLayoutRequest = LayoutRequest::Reset;
	}

	ImGui::DockSpace(mDockspaceId);
	ImGui::End();

	// --- Panels -----------------------------------------------------------------------------

	DrawViewport();
	DrawHierarchy();
	DrawProperties();

	ImGui::Begin("Statistics");
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS:   %.1f", io.Framerate);
	ImGui::Text("Frame: %.3f ms", 1000.0f / io.Framerate);
	ImGui::Separator();
	ImGui::Text("Viewport: %.0f x %.0f", mViewportSize.x, mViewportSize.y);
	ImGui::End();

	DrawConsole();
	DrawProject();
	DrawShortcuts();
	DrawLayoutPopups();
	DrawNewComponentPopup();

	// Commit any in-progress continuous edit (gizmo/slider drag) once the mouse is released.
	if (mHasPending && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		CommitEdit();

	if (mShowDemo)
		ImGui::ShowDemoWindow(&mShowDemo);
}

namespace
{
	// Console severity buckets, mirroring the three filter toggles (Unity-style).
	enum class LogBucket { Error, Warning, Info };

	LogBucket LogLevelBucket(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::Error:
			case LogLevel::Fatal:   return LogBucket::Error;
			case LogLevel::Warning: return LogBucket::Warning;
			default:                return LogBucket::Info;
		}
	}

	// Display color for each log severity, mirroring the spdlog console coloring.
	ImVec4 LogLevelColor(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::Error:       return ImVec4(0.94f, 0.35f, 0.35f, 1.0f);
			case LogLevel::Fatal:       return ImVec4(1.00f, 0.30f, 0.55f, 1.0f);
			case LogLevel::Warning:     return ImVec4(0.95f, 0.80f, 0.35f, 1.0f);
			case LogLevel::Success:     return ImVec4(0.45f, 0.85f, 0.50f, 1.0f);
			case LogLevel::Information: return ImVec4(0.60f, 0.75f, 0.95f, 1.0f);
			case LogLevel::Trace:       return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
		}

		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// A severity filter rendered as a toggle button with its message count.
	bool LogFilterToggle(const char8* label, int count, bool& enabled, const ImVec4& color)
	{
		const std::string text = std::string(label) + "  " + std::to_string(count);

		ImGui::PushStyleColor(ImGuiCol_Button, enabled ? ImVec4(color.x, color.y, color.z, 0.28f) : ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, enabled ? color : ImVec4(0.45f, 0.46f, 0.49f, 1.0f));

		const bool clicked = ImGui::Button(text.c_str());

		ImGui::PopStyleColor(2);

		if (clicked)
			enabled = !enabled;

		return clicked;
	}
}

void EditorLayer::DrawConsole()
{
	// While the panel is collapsed or sits behind another dock tab there is no layout to build, and
	// leaving the tail counter untouched makes it snap to the newest line once it comes back.
	if (!ImGui::Begin("Console"))
	{
		ImGui::End();
		return;
	}

	const std::deque<LogEntry>& history = Log::GetHistory();

	int errorCount = 0, warningCount = 0, infoCount = 0;
	for (const LogEntry& entry : history)
	{
		switch (LogLevelBucket(entry.level))
		{
			case LogBucket::Error:   errorCount++;   break;
			case LogBucket::Warning: warningCount++; break;
			case LogBucket::Info:    infoCount++;    break;
		}
	}

	// --- Toolbar: clear, search, follow-tail, and the severity filters (right-aligned).
	const ImGuiStyle& style = ImGui::GetStyle();

	if (ImGui::Button("Clear"))
		Log::ClearHistory();

	ImGui::SameLine();
	static ImGuiTextFilter filter;
	ImGui::SetNextItemWidth(240.0f);
	if (ImGui::InputTextWithHint("##search", "Search logs...", filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf)))
		filter.Build();

	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &mConsoleAutoScroll);

	const std::string errorText = "ERROR  " + std::to_string(errorCount);
	const std::string warnText = "WARN  " + std::to_string(warningCount);
	const std::string infoText = "INFO  " + std::to_string(infoCount);
	const float32 togglesWidth =
		ImGui::CalcTextSize(errorText.c_str()).x + ImGui::CalcTextSize(warnText.c_str()).x +
		ImGui::CalcTextSize(infoText.c_str()).x + style.FramePadding.x * 6.0f + style.ItemSpacing.x * 2.0f;

	ImGui::SameLine(ImGui::GetContentRegionMax().x - togglesWidth);
	LogFilterToggle("ERROR", errorCount, mConsoleShowErrors, LogLevelColor(LogLevel::Error));
	ImGui::SameLine();
	LogFilterToggle("WARN", warningCount, mConsoleShowWarnings, LogLevelColor(LogLevel::Warning));
	ImGui::SameLine();
	LogFilterToggle("INFO", infoCount, mConsoleShowInfo, LogLevelColor(LogLevel::Information));

	ImGui::Separator();

	// --- Collect the entries that pass the filters; the list below indexes into this.
	mConsoleVisible.clear();

	for (int index = 0; index < static_cast<int>(history.size()); ++index)
	{
		const LogBucket bucket = LogLevelBucket(history[index].level);

		if ((bucket == LogBucket::Error && !mConsoleShowErrors) ||
			(bucket == LogBucket::Warning && !mConsoleShowWarnings) ||
			(bucket == LogBucket::Info && !mConsoleShowInfo))
			continue;

		if (!filter.PassFilter(history[index].message.c_str()))
			continue;

		mConsoleVisible.push_back(index);
	}

	// --- Message list: a severity dot, a dimmed timestamp and the message. The clipper submits only
	// the rows actually on screen, so a full history costs no more than a screenful.
	ImGui::BeginChild("ConsoleOutput", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

	int contextIndex = -1;
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(mConsoleVisible.size()));

	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const int index = mConsoleVisible[row];
			const LogEntry& entry = history[index];
			const ImVec4 accent = LogLevelColor(entry.level);

			ImGui::PushID(index);

			const ImVec2 rowMin = ImGui::GetCursorScreenPos();

			if (ImGui::Selectable("##row", mConsoleSelected == index))
				mConsoleSelected = index;

			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				mConsoleSelected = index;
				contextIndex = index;
			}

			// A small severity dot stands in for the icon that will replace it later.
			drawList->AddCircleFilled(
				ImVec2(rowMin.x + 8.0f, rowMin.y + ImGui::GetTextLineHeight() * 0.5f),
				4.0f, ImGui::GetColorU32(accent));

			ImGui::SameLine(24.0f);
			ImGui::TextDisabled("%s", entry.time.c_str());

			ImGui::SameLine(96.0f);

			// Only errors and warnings tint their message; everything else reads as plain text.
			const bool tinted = LogLevelBucket(entry.level) != LogBucket::Info;

			if (tinted)
				ImGui::PushStyleColor(ImGuiCol_Text, accent);

			ImGui::TextUnformatted(entry.message.c_str());

			if (tinted)
				ImGui::PopStyleColor();

			ImGui::PopID();
		}
	}

	clipper.End();

	if (contextIndex >= 0)
		ImGui::OpenPopup("ConsoleRowContext");

	if (ImGui::BeginPopup("ConsoleRowContext"))
	{
		if (ImGui::MenuItem("Copy message") && mConsoleSelected >= 0 && mConsoleSelected < static_cast<int>(history.size()))
			ImGui::SetClipboardText(history[mConsoleSelected].message.c_str());

		ImGui::EndPopup();
	}

	// Follow the tail whenever lines were logged since the last frame this panel was drawn. Keying
	// off the total logged count rather than the scroll offset keeps the follow alive once the
	// history saturates at its cap, and after the panel spent frames hidden behind another tab
	// (where the offset stays put while the content grows past it). SetScrollHereY targets the
	// cursor, which the clipper leaves at the end of the virtual list, so it lands on the bottom.
	const size_t totalLogged = Log::GetTotalCount();

	if (mConsoleAutoScroll && totalLogged != mConsoleLastTotal)
		ImGui::SetScrollHereY(1.0f);

	mConsoleLastTotal = totalLogged;

	ImGui::EndChild();
	ImGui::End();
}

namespace
{
	// The editor keeps its persisted state (UI layout, shortcuts, saved dock layouts) under Data/,
	// which sits in the resource root. The Project panel skips the whole folder: it is editor state,
	// not an asset.
	constexpr const char8* kDataDirectory    = "Data";
	constexpr const char8* kLayoutsDirectory = "Data/Layouts";

	// Component types the editor adds through a bespoke Add Component entry, because they need
	// construction arguments (a collider sizes itself to the sprite). Everything else in the registry
	// comes from the game module and is added generically, by name.
	constexpr const char8* kBuiltInComponents[] = {
		"SpriteRenderer", "RigidBody2D", "BoxCollider2D", "CircleCollider2D", "ScriptComponent" };

	bool IsBuiltInComponent(const std::string& name)
	{
		for (const char8* entry : kBuiltInComponents)
			if (name == entry)
				return true;

		return false;
	}

	// The game's source tree, relative to the project root — the editor writes generated components
	// into it and recognises the project root by finding it. It is the folder, not the VS project name.
	constexpr const char8* kGameSourceFolder = "Sandbox";

	// The project root, found by walking up from the working directory: the editor runs from its build
	// output, not the project. Empty when the project is not around (a distributed editor), which is
	// what disables generating and compiling.
	std::filesystem::path ProjectRootDirectory()
	{
		std::error_code error;
		std::filesystem::path current = std::filesystem::current_path(error);

		if (error)
			return {};

		for (int32 depth = 0; depth < 8; ++depth)
		{
			if (std::filesystem::is_directory(current / kGameSourceFolder / "Source", error))
				return current;

			if (!current.has_parent_path() || current.parent_path() == current)
				break;

			current = current.parent_path();
		}

		return {};
	}

	// Where a generated component's source lands.
	std::filesystem::path GameComponentDirectory()
	{
		const std::filesystem::path root = ProjectRootDirectory();
		return root.empty() ? root : (root / kGameSourceFolder / "Source" / "Component");
	}

	// Runs a command, capturing whatever it writes to stdout and stderr, and returns its exit code.
	// The whole command is wrapped in an extra pair of quotes: cmd.exe otherwise mangles one that
	// starts with a quoted path.
	int32 RunCommand(const std::string& command, std::string& output)
	{
		const std::string wrapped = "\"" + command + " 2>&1\"";

		FILE* pipe = _popen(wrapped.c_str(), "r");

		if (!pipe)
			return -1;

		char8 buffer[512];

		while (std::fgets(buffer, sizeof(buffer), pipe))
			output += buffer;

		return _pclose(pipe);
	}

	// MSBuild's path is not fixed across Visual Studio installs, so ask the installer's own locator.
	const std::string& MSBuildPath()
	{
		static const std::string path = []
		{
			const char8* programFiles = std::getenv("ProgramFiles(x86)");

			if (!programFiles)
				return std::string();

			const std::string vswhere =
				"\"" + std::string(programFiles) + "\\Microsoft Visual Studio\\Installer\\vswhere.exe\"";

			std::string output;

			if (RunCommand(vswhere + " -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe", output) != 0)
				return std::string();

			// vswhere prints one path per line; take the first and strip the newline.
			const size_t end = output.find_first_of("\r\n");
			return (end == std::string::npos) ? output : output.substr(0, end);
		}();

		return path;
	}

	// The configuration the editor itself was built in: the game module has to match it, or the two
	// would disagree on the runtime and the module could not be loaded.
	constexpr const char8* BuildConfiguration()
	{
#if defined(LN_DEBUG)
		return "Debug";
#elif defined(LN_RELEASE)
		return "Release";
#else
		return "Shipping";
#endif
	}

	// Inspector rows put the label first, in a fixed column, and the widget after it — ImGui's default
	// is the other way round, which reads backwards for a property. The column is wide enough for the
	// longest label in use ("Fixed Rotation"), so every row in every component lines up.
	constexpr float32 kPropertyLabelWidth = 112.0f;

	// Lays out a property row's label and leaves the cursor on the widget. Pass a width for a widget
	// that sizes itself; the default fills the rest of the row.
	void PropertyLabel(const char8* label, float32 widgetWidth = 0.0f)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine(kPropertyLabelWidth);
		ImGui::SetNextItemWidth((widgetWidth > 0.0f) ? widgetWidth : ImGui::GetContentRegionAvail().x);
	}

	// A generated type name becomes a class name and a file name, so hold it to a C++ identifier.
	bool IsValidTypeName(const std::string& name)
	{
		if (name.empty() || name.size() > 48)
			return false;

		if (std::isdigit(static_cast<unsigned char>(name.front())))
			return false;

		return std::all_of(name.begin(), name.end(), [](char8 character)
		{
			return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
		});
	}

	// Only asset-like files are listed; the resource root also holds the executable and its DLLs.
	bool IsAssetFile(const std::filesystem::path& path)
	{
		static const char8* extensions[] = { ".png", ".jpg", ".jpeg", ".bmp", ".glsl", ".json" };

		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { return static_cast<char8>(std::tolower(c)); });

		for (const char8* candidate : extensions)
		{
			if (extension == candidate)
				return true;
		}

		return false;
	}
}

void EditorLayer::DrawProject()
{
	ImGui::Begin("Project");

	const std::string& root = ResourceRootDirectory();

	if (root.empty())
	{
		ImGui::TextDisabled("Could not locate the resource root.");
		ImGui::End();
		return;
	}

	const std::filesystem::path current = std::filesystem::path(root) / mProjectPath;

	// --- Breadcrumb toolbar.
	ImGui::BeginDisabled(mProjectPath.empty());
	if (ImGui::Button("Up"))
	{
		const std::filesystem::path parent = std::filesystem::path(mProjectPath).parent_path();
		mProjectPath = parent.generic_string();
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::TextDisabled("Assets/%s", mProjectPath.c_str());

	ImGui::Separator();

	std::error_code error;

	if (!std::filesystem::is_directory(current, error))
	{
		ImGui::TextDisabled("Folder not found; returning to the root.");
		mProjectPath.clear();
		ImGui::End();
		return;
	}

	// --- Directories first, then asset files.
	std::string enterDirectory;

	for (const auto& entry : std::filesystem::directory_iterator(current, error))
	{
		if (!entry.is_directory())
			continue;

		const std::string name = entry.path().filename().generic_string();

		// The editor's own Data folder sits in the resource root, but it holds no assets.
		if (mProjectPath.empty() && name == kDataDirectory)
			continue;

		ImGui::PushID(name.c_str());
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.80f, 0.35f, 1.0f));
		const bool activated = ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
		ImGui::PopStyleColor();

		if (activated && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			enterDirectory = mProjectPath.empty() ? name : mProjectPath + "/" + name;

		ImGui::PopID();
	}

	for (const auto& entry : std::filesystem::directory_iterator(current, error))
	{
		if (entry.is_directory() || !IsAssetFile(entry.path()))
			continue;

		const std::string name = entry.path().filename().generic_string();
		const std::string assetPath = mProjectPath.empty() ? name : mProjectPath + "/" + name;

		ImGui::PushID(name.c_str());
		ImGui::Selectable(name.c_str());

		// Drag an asset onto a field that accepts it (e.g. the Sprite Renderer's texture).
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("LN_ASSET_PATH", assetPath.c_str(), assetPath.size() + 1);
			ImGui::TextUnformatted(name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Copy path"))
				ImGui::SetClipboardText(assetPath.c_str());

			ImGui::EndPopup();
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", assetPath.c_str());

		ImGui::PopID();
	}

	if (!enterDirectory.empty())
		mProjectPath = enterDirectory;

	ImGui::End();
}

void EditorLayer::DrawShortcuts()
{
	if (!mShowShortcuts)
		return;

	ImGui::SetNextWindowSize(ImVec2(460.0f, 500.0f), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Shortcuts", &mShowShortcuts))
	{
		// The rebindable actions, grouped by category (order defines the display order).
		struct Row { ShortcutAction action; const char8* category; const char8* name; };
		static const Row rows[] = {
			{ ShortcutAction::ToggleShortcuts, "General",   "Toggle Shortcuts panel" },
			{ ShortcutAction::Undo,            "General",   "Undo" },
			{ ShortcutAction::Redo,            "General",   "Redo" },
			{ ShortcutAction::Play,            "Play Mode", "Play (run the simulation)" },
			{ ShortcutAction::Pause,           "Play Mode", "Pause / resume the simulation" },
			{ ShortcutAction::Stop,            "Play Mode", "Stop (return to edited state)" },
			{ ShortcutAction::ToolSelect,      "Tools",     "Select tool" },
			{ ShortcutAction::GizmoMove,       "Tools",     "Move tool (translate)" },
			{ ShortcutAction::GizmoRotate,     "Tools",     "Rotate tool" },
			{ ShortcutAction::GizmoScale,      "Tools",     "Scale tool" },
			{ ShortcutAction::RenameEntity,    "Hierarchy", "Rename selected entity" },
			{ ShortcutAction::DeleteEntity,    "Hierarchy", "Delete selected entity" },
			{ ShortcutAction::CopyEntity,      "Hierarchy", "Copy selected entity" },
			{ ShortcutAction::PasteEntity,     "Hierarchy", "Paste entity from clipboard" },
			{ ShortcutAction::DuplicateEntity, "Hierarchy", "Duplicate selected entity" },
			{ ShortcutAction::StepFrame,       "Play Mode", "Step one frame" },
			{ ShortcutAction::ToggleColliders, "Viewport",  "Toggle collider hitboxes" },
			{ ShortcutAction::CompileModule,   "Game",      "Compile the game module" },
			{ ShortcutAction::ReloadModule,    "Game",      "Reload the game module" },
		};

		ImGui::TextDisabled("Click a shortcut to rebind it, then press a key (Esc to cancel).");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Reset to Defaults").x - ImGui::GetStyle().FramePadding.x * 2.0f);
		if (ImGui::SmallButton("Reset to Defaults"))
		{
			ResetShortcutsToDefault();
			SaveShortcuts();
			mRebindingIndex = -1;
		}

		const char8* currentCategory = nullptr;

		for (const Row& row : rows)
		{
			const int index = static_cast<int>(row.action);

			if (currentCategory == nullptr || std::string(currentCategory) != row.category)
			{
				currentCategory = row.category;
				ImGui::SeparatorText(row.category);
			}

			ImGui::PushID(index);

			// A fixed-width button on the left showing the current binding; click to rebind.
			const bool capturing = (mRebindingIndex == index);
			const std::string label = capturing ? "Press a key..." : KeybindToString(mBinds[index]);

			if (capturing)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.58f, 0.95f, 1.0f));

			if (ImGui::Button(label.c_str(), ImVec2(150.0f, 0.0f)))
				mRebindingIndex = capturing ? -1 : index;

			if (capturing)
				ImGui::PopStyleColor();

			ImGui::SameLine();
			ImGui::TextUnformatted(row.name);

			ImGui::PopID();
		}

		// Non-rebindable, mouse-driven shortcuts, shown for reference.
		ImGui::SeparatorText("Mouse");
		ImGui::BulletText("Left Click viewport - select entity");
		ImGui::BulletText("Double-click hierarchy - rename entity");
		ImGui::BulletText("Right-click hierarchy - context menu");
		ImGui::BulletText("Drag component header - reorder");

		// Capture a new key for the action being rebound.
		if (mRebindingIndex >= 0)
		{
			const ImGuiIO& io = ImGui::GetIO();

			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				mRebindingIndex = -1;
			}
			else
			{
				const auto isModifier = [](ImGuiKey k)
				{
					return k == ImGuiKey_LeftCtrl  || k == ImGuiKey_RightCtrl
						|| k == ImGuiKey_LeftShift || k == ImGuiKey_RightShift
						|| k == ImGuiKey_LeftAlt   || k == ImGuiKey_RightAlt
						|| k == ImGuiKey_LeftSuper || k == ImGuiKey_RightSuper;
				};

				for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = static_cast<ImGuiKey>(key + 1))
				{
					if (isModifier(key) || !ImGui::IsKeyPressed(key, false))
						continue;

					// Skip mouse and gamepad "keys"; only accept keyboard keys.
					const std::string keyName = ImGui::GetKeyName(key);
					if (keyName.rfind("Mouse", 0) == 0 || keyName.rfind("Pad", 0) == 0 || keyName.rfind("Gamepad", 0) == 0)
						continue;

					mBinds[mRebindingIndex] = { key, io.KeyCtrl, io.KeyShift, io.KeyAlt };
					mRebindingIndex = -1;
					SaveShortcuts();
					break;
				}
			}
		}
	}

	ImGui::End();
}

void EditorLayer::RecordSnapshot()
{
	// Snapshot the current state onto the undo stack (used right before a discrete edit).
	mUndoStack.push_back(SceneSerializer::SerializeToString(mScene));

	if (mUndoStack.size() > kMaxUndo)
		mUndoStack.erase(mUndoStack.begin());

	mRedoStack.clear();
}

void EditorLayer::BeginEdit()
{
	// Capture the pre-edit state once at the start of a continuous edit (gizmo/slider drag).
	if (mHasPending)
		return;

	mPendingSnapshot = SceneSerializer::SerializeToString(mScene);
	mHasPending = true;
}

void EditorLayer::CommitEdit()
{
	if (!mHasPending)
		return;

	mHasPending = false;

	// Only record a step if the edit actually changed the scene.
	if (SceneSerializer::SerializeToString(mScene) == mPendingSnapshot)
		return;

	mUndoStack.push_back(mPendingSnapshot);

	if (mUndoStack.size() > kMaxUndo)
		mUndoStack.erase(mUndoStack.begin());

	mRedoStack.clear();
}

void EditorLayer::Undo()
{
	if (mUndoStack.empty())
		return;

	mRedoStack.push_back(SceneSerializer::SerializeToString(mScene));

	const std::string state = mUndoStack.back();
	mUndoStack.pop_back();

	// The scene is rebuilt from scratch, so any selected-entity pointer becomes stale.
	mSelectedEntity = nullptr;
	SceneSerializer::DeserializeFromString(mScene, state);
}

void EditorLayer::Redo()
{
	if (mRedoStack.empty())
		return;

	mUndoStack.push_back(SceneSerializer::SerializeToString(mScene));

	const std::string state = mRedoStack.back();
	mRedoStack.pop_back();

	mSelectedEntity = nullptr;
	SceneSerializer::DeserializeFromString(mScene, state);
}

namespace
{
	// User-customized shortcuts, kept under Data/ alongside the UI layout (see EditorGui::Init).
	constexpr const char8* kShortcutsFile = "Data/lion-shortcuts.ini";
}

void EditorLayer::ResetShortcutsToDefault()
{
	const auto set = [&](ShortcutAction action, ImGuiKey key, bool ctrl = false, bool shift = false, bool alt = false)
	{
		mBinds[static_cast<int>(action)] = { key, ctrl, shift, alt };
	};

	set(ShortcutAction::Undo, ImGuiKey_Z, true);
	set(ShortcutAction::Redo, ImGuiKey_Y, true);
	set(ShortcutAction::Play, ImGuiKey_F5);
	set(ShortcutAction::Stop, ImGuiKey_F8);
	set(ShortcutAction::ToggleShortcuts, ImGuiKey_F1);
	set(ShortcutAction::GizmoMove, ImGuiKey_W);
	set(ShortcutAction::GizmoRotate, ImGuiKey_E);
	set(ShortcutAction::GizmoScale, ImGuiKey_R);
	set(ShortcutAction::RenameEntity, ImGuiKey_F2);
	set(ShortcutAction::DeleteEntity, ImGuiKey_Delete);
	set(ShortcutAction::Pause, ImGuiKey_F7);
	set(ShortcutAction::ToggleColliders, ImGuiKey_F4);
	set(ShortcutAction::CopyEntity, ImGuiKey_C, true);
	set(ShortcutAction::PasteEntity, ImGuiKey_V, true);
	set(ShortcutAction::DuplicateEntity, ImGuiKey_D, true);
	set(ShortcutAction::ToolSelect, ImGuiKey_Q);
	set(ShortcutAction::StepFrame, ImGuiKey_F6);

	// Borrowed from Visual Studio, where Ctrl+Shift+B builds; reload sits next to it.
	set(ShortcutAction::CompileModule, ImGuiKey_B, true, true);
	set(ShortcutAction::ReloadModule, ImGuiKey_R, true, true);
}

void EditorLayer::CreateFolder()
{
	RecordSnapshot();

	auto folder = MakeReference<Entity>();
	folder->SetFolder(true);
	folder->SetName("Folder");
	mScene->Add(folder);

	mSelectedEntity = folder;
	mRenamingEntity = folder;   // Let the user name it right away.
	mRenameFocus = true;
}

void EditorLayer::CopyEntity()
{
	if (mSelectedEntity)
		mEntityClipboard = SceneSerializer::SerializeEntityToString(mSelectedEntity);
}

void EditorLayer::PasteEntity()
{
	if (mEntityClipboard.empty())
		return;

	RecordSnapshot();

	if (Reference<Entity> pasted = SceneSerializer::DeserializeEntityFromString(mScene, mEntityClipboard))
	{
		pasted->SetName(pasted->GetName() + " (Copy)");
		mSelectedEntity = pasted;
	}
}

void EditorLayer::DuplicateEntity()
{
	if (!mSelectedEntity)
		return;

	RecordSnapshot();

	const std::string data = SceneSerializer::SerializeEntityToString(mSelectedEntity);

	if (Reference<Entity> copy = SceneSerializer::DeserializeEntityFromString(mScene, data))
	{
		copy->SetName(copy->GetName() + " (Copy)");
		mSelectedEntity = copy;
	}
}

void EditorLayer::InitShortcuts()
{
	ResetShortcutsToDefault();
	LoadShortcuts();
}

void EditorLayer::LoadShortcuts()
{
	std::ifstream file(kShortcutsFile);

	if (!file.is_open())
		return;

	int index = 0, key = 0, ctrl = 0, shift = 0, alt = 0;

	while (file >> index >> key >> ctrl >> shift >> alt)
	{
		if (index >= 0 && index < static_cast<int>(ShortcutAction::Count))
			mBinds[index] = { static_cast<ImGuiKey>(key), ctrl != 0, shift != 0, alt != 0 };
	}
}

void EditorLayer::SaveShortcuts() const
{
	// Data/ normally already exists (EditorGui::Init makes it), but keep this self-contained.
	std::error_code error;
	std::filesystem::create_directories("Data", error);

	std::ofstream file(kShortcutsFile);

	if (!file.is_open())
		return;

	for (int i = 0; i < static_cast<int>(ShortcutAction::Count); ++i)
	{
		const Keybind& bind = mBinds[i];
		file << i << ' ' << static_cast<int>(bind.key) << ' '
			<< (bind.ctrl ? 1 : 0) << ' ' << (bind.shift ? 1 : 0) << ' ' << (bind.alt ? 1 : 0) << '\n';
	}
}

bool EditorLayer::IsShortcutPressed(ShortcutAction action) const
{
	const Keybind& bind = mBinds[static_cast<int>(action)];

	if (bind.key == ImGuiKey_None)
		return false;

	const ImGuiIO& io = ImGui::GetIO();

	if (io.KeyCtrl != bind.ctrl || io.KeyShift != bind.shift || io.KeyAlt != bind.alt)
		return false;

	return ImGui::IsKeyPressed(bind.key, false);
}

std::string EditorLayer::KeybindToString(const Keybind& bind) const
{
	if (bind.key == ImGuiKey_None)
		return "None";

	std::string text;
	if (bind.ctrl)  text += "Ctrl+";
	if (bind.shift) text += "Shift+";
	if (bind.alt)   text += "Alt+";
	text += ImGui::GetKeyName(bind.key);
	return text;
}

void EditorLayer::HandleShortcuts()
{
	// Don't trigger actions while the user is capturing a new key in the Shortcuts panel.
	if (mRebindingIndex >= 0)
		return;

	// A modal owns the keyboard for as long as it is up — whether or not one of its fields happens to
	// hold focus at that moment. Without this, a class name typed into the New C++ Component dialog
	// also ran whatever the editor binds those letters to, so an "e" would switch the gizmo tool.
	//
	// An active text field owns it for the same reason: any action can be rebound to a plain letter,
	// so nothing may be exempt from this on the grounds of currently being a function key.
	if (ImGui::GetTopMostPopupModal() != nullptr || ImGui::GetIO().WantTextInput)
		return;

	if (IsShortcutPressed(ShortcutAction::Play)) StartPlay();
	if (IsShortcutPressed(ShortcutAction::Pause)) TogglePause();
	if (IsShortcutPressed(ShortcutAction::Stop)) StopPlay();
	if (IsShortcutPressed(ShortcutAction::ToggleShortcuts)) mShowShortcuts = !mShowShortcuts;
	if (IsShortcutPressed(ShortcutAction::ToggleColliders)) mShowColliders = !mShowColliders;
	if (IsShortcutPressed(ShortcutAction::StepFrame)) StepOneFrame();
	if (IsShortcutPressed(ShortcutAction::CompileModule)) CompileGameModule();
	if (IsShortcutPressed(ShortcutAction::ReloadModule)) ReloadGameModule();

	// The actions below are edit-mode only (typing is already ruled out above).
	if (mPlaying)
		return;

	// The bound tool keys pick the active viewport tool. A gizmo drag or an active widget owns the
	// input while it lasts, so the keys do not fire underneath them.
	if (!ImGuizmo::IsUsing() && !ImGui::IsAnyItemActive())
	{
		if (IsShortcutPressed(ShortcutAction::ToolSelect))  mTool = Tool::Select;
		if (IsShortcutPressed(ShortcutAction::GizmoMove))   mTool = Tool::Move;
		if (IsShortcutPressed(ShortcutAction::GizmoRotate)) mTool = Tool::Rotate;
		if (IsShortcutPressed(ShortcutAction::GizmoScale))  mTool = Tool::Scale;
	}

	if (IsShortcutPressed(ShortcutAction::Undo)) Undo();
	if (IsShortcutPressed(ShortcutAction::Redo)) Redo();

	if (IsShortcutPressed(ShortcutAction::CopyEntity)) CopyEntity();
	if (IsShortcutPressed(ShortcutAction::PasteEntity)) PasteEntity();
	if (IsShortcutPressed(ShortcutAction::DuplicateEntity)) DuplicateEntity();

	if (mSelectedEntity && IsShortcutPressed(ShortcutAction::RenameEntity))
	{
		mRenamingEntity = mSelectedEntity;
		mRenameFocus = true;
	}

	if (mSelectedEntity && IsShortcutPressed(ShortcutAction::DeleteEntity))
	{
		RecordSnapshot();
		mScene->Remove(mSelectedEntity);
		mScene->FlushRemovals();
		mSelectedEntity = nullptr;
	}
}

void EditorLayer::DrawViewport()
{
	// The viewport image fills the window edge to edge. Begin() captures the padding, so it is
	// restored right away: popups opened from here (e.g. Settings) then get the normal padding.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Viewport");
	ImGui::PopStyleVar();

	const ImVec2 available = ImGui::GetContentRegionAvail();
	mViewportSize = { available.x, available.y };

	// Display the framebuffer's color texture, flipped vertically (OpenGL is bottom-up).
	const auto textureId = static_cast<ImTextureID>(mFramebuffer->GetColorAttachment());
	ImGui::Image(textureId, available, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

	const ImVec2 imageMin = ImGui::GetItemRectMin();
	const ImVec2 imageSize = ImGui::GetItemRectSize();
	const bool imageHovered = ImGui::IsItemHovered();

	// The gizmo is an editing tool; hide it while the simulation is running, for folders (no
	// meaningful transform), and for the Select tool, which only picks entities.
	if (mSelectedEntity && !mPlaying && mTool != Tool::Select && !mSelectedEntity->IsFolder())
	{
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

		const glm::mat4 view = mCamera->GetViewMatrix();
		const glm::mat4 projection = mCamera->GetProjectionMatrix();

		// The gizmo manipulates the entity in world space; the results are rebased onto its local
		// transform, so a child keeps following its parent.
		const Vector position = mSelectedEntity->GetWorldPosition();
		const float32 rotationZ = mSelectedEntity->GetWorldRotation();
		const Vector scale = mSelectedEntity->GetWorldScale();

		// Build the entity's model matrix (rotation in degrees, matching the Transform).
		float32 translationValues[3] = { position.x, position.y, position.z };
		float32 rotationValues[3] = { 0.0f, 0.0f, rotationZ };
		float32 scaleValues[3] = { scale.x, scale.y, 1.0f };

		float32 model[16];
		ImGuizmo::RecomposeMatrixFromComponents(translationValues, rotationValues, scaleValues, model);

		// Capture the pre-edit state the moment the gizmo is grabbed (before Manipulate mutates it).
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGuizmo::IsOver())
			BeginEdit();

		const ImGuizmo::OPERATION operation =
			(mTool == Tool::Rotate) ? ImGuizmo::ROTATE :
			(mTool == Tool::Scale)  ? ImGuizmo::SCALE  : ImGuizmo::TRANSLATE;

		ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation, ImGuizmo::LOCAL, model);

		if (ImGuizmo::IsUsing())
		{
			float32 newTranslation[3], newRotation[3], newScale[3];
			ImGuizmo::DecomposeMatrixToComponents(model, newTranslation, newRotation, newScale);

			mSelectedEntity->SetWorldPosition(Vector(newTranslation[0], newTranslation[1], position.z));
			mSelectedEntity->SetWorldRotation(newRotation[2]);
			mSelectedEntity->SetWorldScale(Vector(newScale[0], newScale[1], scale.z));
		}
	}

	if (mShowColliders)
		DrawColliderOverlays(imageMin, imageSize);

	// Play mode is signalled by an accent outline around the viewport. It is inset by half its
	// thickness so the whole stroke stays inside the image instead of being clipped in half.
	if (mPlaying)
	{
		constexpr float32 thickness = 4.0f;
		const float32 inset = thickness * 0.5f;

		ImGui::GetWindowDrawList()->AddRect(
			ImVec2(imageMin.x + inset, imageMin.y + inset),
			ImVec2(imageMin.x + imageSize.x - inset, imageMin.y + imageSize.y - inset),
			IM_COL32(61, 133, 224, 255), 0.0f, 0, thickness);
	}

	DrawViewportToolbar(imageMin, imageSize);

	// Pixel-perfect click-to-select: read the entity id under the cursor from the id attachment.
	// Skipped while over/using the gizmo or the overlay toolbar (those clicks belong to them).
	if (imageHovered && !ImGui::IsAnyItemHovered() &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
	{
		const ImVec2 mouse = ImGui::GetMousePos();
		const int32 pixelX = static_cast<int32>(mouse.x - imageMin.x);
		const int32 pixelY = static_cast<int32>(imageSize.y - (mouse.y - imageMin.y));  // Flip Y (bottom-up).

		if (pixelX >= 0 && pixelY >= 0 && pixelX < static_cast<int32>(imageSize.x) && pixelY < static_cast<int32>(imageSize.y))
		{
			const int32 id = mFramebuffer->ReadEntityId(static_cast<uint32>(pixelX), static_cast<uint32>(pixelY));

			mSelectedEntity = nullptr;
			for (const auto& entity : mScene->GetEntities())
			{
				if (entity->GetId() == id)
				{
					mSelectedEntity = entity;
					break;
				}
			}
		}
	}

	ImGui::End();
}

void EditorLayer::DrawViewportToolbar(const ImVec2& imageMin, const ImVec2& imageSize)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	const ImVec4 accent(0.24f, 0.52f, 0.90f, 1.0f);

	// Tools (Select / Move / Rotate / Scale), pinned to the viewport's top-left corner. Icons will
	// replace the labels later.
	ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 8.0f, imageMin.y + 8.0f));

	struct ToolButton { Tool tool; const char8* label; const char8* tooltip; };
	static const ToolButton tools[] = {
		{ Tool::Select, "Select", "Select entities (Q)" },
		{ Tool::Move,   "Move",   "Move the selection (W)" },
		{ Tool::Rotate, "Rotate", "Rotate the selection (E)" },
		{ Tool::Scale,  "Scale",  "Scale the selection (R)" },
	};

	for (const ToolButton& button : tools)
	{
		if (button.tool != tools[0].tool)
			ImGui::SameLine();

		const bool active = (mTool == button.tool);

		if (active)
			ImGui::PushStyleColor(ImGuiCol_Button, accent);

		if (ImGui::Button(button.label))
			mTool = button.tool;

		if (active)
			ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", button.tooltip);
	}

	// Settings, pinned to the viewport's top-right corner (a gear icon later).
	const float32 settingsWidth = ImGui::CalcTextSize("Settings").x + style.FramePadding.x * 2.0f;
	ImGui::SetCursorScreenPos(ImVec2(imageMin.x + imageSize.x - settingsWidth - 8.0f, imageMin.y + 8.0f));

	if (ImGui::Button("Settings"))
		ImGui::OpenPopup("ViewportSettings");

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Viewport display options");

	if (ImGui::BeginPopup("ViewportSettings"))
	{
		// Shading modes are placeholders until the renderer supports them.
		bool unavailable = false;
		ImGui::BeginDisabled();
		ImGui::MenuItem("Lit", nullptr, &unavailable);
		ImGui::MenuItem("Unlit", nullptr, &unavailable);
		ImGui::MenuItem("Wireframe", nullptr, &unavailable);
		ImGui::EndDisabled();

		ImGui::Separator();
		ImGui::MenuItem("Colliders", "F4", &mShowColliders);

		ImGui::EndPopup();
	}

	// Play / Step / Pause / Stop, centered along the viewport's top edge.
	const char8* pauseLabel = mPaused ? "Resume" : "Pause";
	const float32 playWidth = ImGui::CalcTextSize("Play").x + style.FramePadding.x * 2.0f;
	const float32 stepWidth = ImGui::CalcTextSize("Step").x + style.FramePadding.x * 2.0f;
	const float32 pauseWidth = ImGui::CalcTextSize(pauseLabel).x + style.FramePadding.x * 2.0f;
	const float32 stopWidth = ImGui::CalcTextSize("Stop").x + style.FramePadding.x * 2.0f;
	const float32 totalWidth = playWidth + stepWidth + pauseWidth + stopWidth + style.ItemSpacing.x * 3.0f;

	ImGui::SetCursorScreenPos(ImVec2(imageMin.x + (imageSize.x - totalWidth) * 0.5f, imageMin.y + 8.0f));

	// Play doubles as "resume" while paused, so it stays enabled in that state.
	ImGui::BeginDisabled(mPlaying && !mPaused);
	if (ImGui::Button("Play"))
		StartPlay();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Run the scene simulation (F5)");

	// Step advances a halted simulation one frame at a time, so it only means anything while playing.
	ImGui::SameLine();
	ImGui::BeginDisabled(!mPlaying);
	if (ImGui::Button("Step"))
		StepOneFrame();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Advance the simulation by one frame (F6)");

	ImGui::SameLine();
	ImGui::BeginDisabled(!mPlaying);
	if (mPaused)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.52f, 0.90f, 1.0f));
	if (ImGui::Button(pauseLabel))
		TogglePause();
	if (mPaused)
		ImGui::PopStyleColor();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(mPaused ? "Resume the simulation (F7)" : "Pause the simulation (F7)");

	ImGui::SameLine();
	ImGui::BeginDisabled(!mPlaying);
	if (ImGui::Button("Stop"))
		StopPlay();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Stop and return to the edited state (F8)");
}

void EditorLayer::DrawColliderOverlays(const ImVec2& imageMin, const ImVec2& imageSize)
{
	if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
		return;

	const glm::mat4 viewProjection = mCamera->GetProjectionMatrix() * mCamera->GetViewMatrix();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 color = IM_COL32(80, 220, 120, 255);  // Unity-like collider green.

	// Projects a world-space point (in pixels, z = 0) to a screen position inside the viewport image.
	const auto worldToScreen = [&](float32 worldX, float32 worldY) -> ImVec2
	{
		const glm::vec4 clip = viewProjection * glm::vec4(worldX, worldY, 0.0f, 1.0f);
		const float32 ndcX = clip.x / clip.w;
		const float32 ndcY = clip.y / clip.w;
		return ImVec2(
			imageMin.x + (ndcX * 0.5f + 0.5f) * imageSize.x,
			imageMin.y + (1.0f - (ndcY * 0.5f + 0.5f)) * imageSize.y);  // Flip Y for top-down screen space.
	};

	for (const auto& entity : mScene->GetEntities())
	{
		// Hitboxes follow the entity's world transform (including anything inherited from a parent).
		const Vector position = entity->GetWorldPosition();
		const Vector scale = entity->GetWorldScale();
		const float32 angle = glm::radians(entity->GetWorldRotation());
		const float32 cosAngle = std::cos(angle);
		const float32 sinAngle = std::sin(angle);

		// Collider sizes are unscaled pixels; apply the Transform scale, matching the physics shapes.
		if (const BoxCollider2D* box = entity->GetComponent<BoxCollider2D>())
		{
			const float32 halfWidth = box->GetWidth() * 0.5f * std::fabs(scale.x);
			const float32 halfHeight = box->GetHeight() * 0.5f * std::fabs(scale.y);

			const auto corner = [&](float32 offsetX, float32 offsetY)
			{
				return worldToScreen(
					position.x + offsetX * cosAngle - offsetY * sinAngle,
					position.y + offsetX * sinAngle + offsetY * cosAngle);
			};

			const ImVec2 points[4] = {
				corner(-halfWidth,  halfHeight), corner(halfWidth,  halfHeight),
				corner( halfWidth, -halfHeight), corner(-halfWidth, -halfHeight),
			};
			drawList->AddPolyline(points, 4, color, ImDrawFlags_Closed, 1.5f);
		}

		if (const CircleCollider2D* circle = entity->GetComponent<CircleCollider2D>())
		{
			const float32 radius = circle->GetRadius() * std::max(std::fabs(scale.x), std::fabs(scale.y));
			const ImVec2 center = worldToScreen(position.x, position.y);
			const ImVec2 edge = worldToScreen(position.x + radius, position.y);
			const float32 screenRadius = std::fabs(edge.x - center.x);
			drawList->AddCircle(center, screenRadius, color, 32, 1.5f);
		}
	}
}

void EditorLayer::DrawHierarchy()
{
	ImGui::Begin("Scene Hierarchy");

	// Compact toolbar: a single "+" create button; everything else is on the context menus.
	if (ImGui::Button("+ Create"))
	{
		RecordSnapshot();
		auto entity = MakeReference<Entity>();
		mScene->Add(entity);
		mSelectedEntity = entity;
		mRenamingEntity = entity;   // Let the user name it right away.
		mRenameFocus = true;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Create an empty entity");

	ImGui::Separator();

	// Children are stored as raw pointers; this maps them back to the scene's owning references.
	mEntityLookup.clear();
	for (const auto& entity : mScene->GetEntities())
		mEntityLookup.emplace(entity.get(), entity);

	mEntityToDelete = nullptr;
	mReparentChild = nullptr;
	mReparentTarget = nullptr;
	mReparentRequested = false;

	// Only roots are drawn at the top level; each node recurses into its children.
	int32 count = 0;
	for (const auto& entity : mScene->GetEntities())
	{
		if (entity->GetParent() == nullptr)
			DrawEntityNode(entity);

		count++;
	}

	if (count == 0)
		ImGui::TextDisabled("Empty scene — right-click to create an entity.");

	// Right-click empty space in the panel to create an entity (Unity/Hazel-style).
	if (ImGui::BeginPopupContextWindow("HierarchyContext",
		ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Create Empty Entity"))
		{
			RecordSnapshot();
			auto entity = MakeReference<Entity>();
			mScene->Add(entity);
			mSelectedEntity = entity;
			mRenamingEntity = entity;
			mRenameFocus = true;
		}

		if (ImGui::MenuItem("Create Folder"))
			CreateFolder();

		if (ImGui::MenuItem("Paste", "Ctrl+V", false, !mEntityClipboard.empty()))
			PasteEntity();

		ImGui::EndPopup();
	}

	// Click empty space inside the panel to deselect.
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		mSelectedEntity = nullptr;

	// Deferred hierarchy edits (never mutate the tree while iterating it above).
	if (mReparentRequested && mReparentChild)
	{
		RecordSnapshot();
		mReparentChild->SetParent(mReparentTarget);
	}

	if (mEntityToDelete)
	{
		RecordSnapshot();

		if (mSelectedEntity == mEntityToDelete) mSelectedEntity = nullptr;
		if (mRenamingEntity == mEntityToDelete) mRenamingEntity = nullptr;

		mScene->Remove(mEntityToDelete);
		mScene->FlushRemovals();
		mEntityToDelete = nullptr;
	}

	ImGui::End();
}

void EditorLayer::DrawEntityNode(const Reference<Entity>& entity)
{
	ImGui::PushID(entity->GetId());

	if (entity == mRenamingEntity)
	{
		// Inline rename field (started via F2, double-click or the context menu).
		char buffer[128];
		const std::string& name = entity->GetName();
		const size_t length = name.copy(buffer, sizeof(buffer) - 1);
		buffer[length] = '\0';

		if (mRenameFocus)
		{
			ImGui::SetKeyboardFocusHere();
			mRenameFocus = false;
		}

		ImGui::SetNextItemWidth(-1.0f);
		const bool committed = ImGui::InputText("##rename", buffer, sizeof(buffer),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

		if (committed || ImGui::IsItemDeactivated())
		{
			RecordSnapshot();
			entity->SetName(buffer);
			mRenamingEntity = nullptr;
		}

		ImGui::PopID();
		return;
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	if (entity->GetChildren().empty())
		flags |= ImGuiTreeNodeFlags_Leaf;

	if (entity == mSelectedEntity)
		flags |= ImGuiTreeNodeFlags_Selected;

	const std::string& name = entity->GetName();
	const bool folder = entity->IsFolder();

	// Folders are tinted so they read as organizational nodes rather than scene objects.
	if (folder)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.80f, 0.35f, 1.0f));

	const bool open = ImGui::TreeNodeEx("##node", flags, "%s", name.empty() ? "(unnamed)" : name.c_str());

	if (folder)
		ImGui::PopStyleColor();

	// Clicking the label (not the expand arrow) selects the entity.
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		mSelectedEntity = entity;

	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		mSelectedEntity = entity;
		mRenamingEntity = entity;
		mRenameFocus = true;
	}

	// Drag a node onto another to reparent it (the child keeps its world transform).
	if (ImGui::BeginDragDropSource())
	{
		Entity* dragged = entity.get();
		ImGui::SetDragDropPayload("LN_ENTITY", &dragged, sizeof(Entity*));
		ImGui::Text("Move %s", name.c_str());
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LN_ENTITY"))
		{
			mReparentChild = *static_cast<Entity* const*>(payload->Data);
			mReparentTarget = entity.get();
			mReparentRequested = true;
		}

		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem())
	{
		mSelectedEntity = entity;

		if (ImGui::MenuItem("Rename", "F2"))
		{
			mRenamingEntity = entity;
			mRenameFocus = true;
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Copy", "Ctrl+C"))      CopyEntity();
		if (ImGui::MenuItem("Duplicate", "Ctrl+D")) DuplicateEntity();
		if (ImGui::MenuItem("Paste", "Ctrl+V", false, !mEntityClipboard.empty())) PasteEntity();

		ImGui::Separator();

		if (ImGui::MenuItem("Unparent", nullptr, false, entity->GetParent() != nullptr))
		{
			mReparentChild = entity.get();
			mReparentTarget = nullptr;
			mReparentRequested = true;
		}

		if (ImGui::MenuItem("Delete", "Del"))
			mEntityToDelete = entity;

		ImGui::EndPopup();
	}

	if (open)
	{
		for (Entity* child : entity->GetChildren())
		{
			const auto it = mEntityLookup.find(child);

			if (it != mEntityLookup.end())
				DrawEntityNode(it->second);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

bool EditorLayer::DrawComponentHeader(const char* label, int index, bool& removeRequested, int& dragFrom, int& dragTo)
{
	ImGui::PushID(label);

	const ImGuiStyle& style = ImGui::GetStyle();
	const float32 lineHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

	ImGui::SetNextItemAllowOverlap();
	const bool open = ImGui::CollapsingHeader(label,
		ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap);

	// Exact header bounds, so the remove button sits flush against its right edge.
	const ImVec2 headerMin = ImGui::GetItemRectMin();
	const ImVec2 headerMax = ImGui::GetItemRectMax();

	// Drag the header onto another component's header to reorder.
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		ImGui::SetDragDropPayload("LN_COMPONENT", &index, sizeof(int));
		ImGui::Text("Move %s", label);
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LN_COMPONENT"))
		{
			dragFrom = *static_cast<const int*>(payload->Data);
			dragTo = index;
		}
		ImGui::EndDragDropTarget();
	}

	// Square "X" button sitting on the header row, flush with its right edge.
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImVec2(headerMax.x - lineHeight, headerMin.y));
	if (ImGui::Button("X", ImVec2(lineHeight, lineHeight)))
		removeRequested = true;

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Remove component");

	ImGui::PopID();
	return open;
}

bool EditorLayer::DrawVec3Control(const char* label, float values[3], float speed, float resetValue)
{
	struct Axis { const char8* name; ImVec4 color; ImVec4 hovered; };
	static const Axis axes[3] = {
		{ "X", ImVec4(0.71f, 0.24f, 0.27f, 1.0f), ImVec4(0.83f, 0.31f, 0.34f, 1.0f) },  // Red
		{ "Y", ImVec4(0.29f, 0.53f, 0.29f, 1.0f), ImVec4(0.36f, 0.64f, 0.36f, 1.0f) },  // Green
		{ "Z", ImVec4(0.22f, 0.40f, 0.68f, 1.0f), ImVec4(0.28f, 0.49f, 0.80f, 1.0f) },  // Blue
	};

	bool changed = false;
	const ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushID(label);

	// A small square badge, rounded like the field beside it. Its frame padding is zeroed so the letter
	// is centred in the whole badge instead of inside a rectangle shrunk by the padding (which, being
	// narrower than the glyph, pushed the letter to the left). The side is rounded down to an even
	// number of pixels, keeping it on the same grid as the rest of the metrics; being shorter than the
	// field, it is nudged down to sit centred against it.
	const float32 frameHeight = ImGui::GetFrameHeight();
	const float32 badge = ImFloor(frameHeight * 0.4f) * 2.0f;
	const float32 badgeOffset = ImFloor((frameHeight - badge) * 0.5f);
	const ImVec2 buttonSize(badge, badge);

	const float32 gap = 4.0f;  // Between a badge and its field.

	// The label takes its column, then the rest is split across the three axes.
	const float32 controlsWidth = ImGui::GetContentRegionAvail().x - kPropertyLabelWidth;
	const float32 axisWidth = (controlsWidth - 2.0f * style.ItemInnerSpacing.x) / 3.0f;
	const float32 dragWidth = ImMax(axisWidth - badge - gap, 12.0f);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine(kPropertyLabelWidth);

	// Every item on this row is placed against this baseline: the badges are offset down from it, and
	// the fields sit on it, so a SameLine cannot drift them apart.
	const float32 rowY = ImGui::GetCursorPosY();

	for (int32 i = 0; i < 3; ++i)
	{
		if (i > 0)
			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

		ImGui::PushID(i);

		// Colored axis badge: click to reset this component.
		ImGui::SetCursorPosY(rowY + badgeOffset);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Button, axes[i].color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, axes[i].hovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, axes[i].color);

		if (ImGui::Button(axes[i].name, buttonSize))
		{
			RecordSnapshot();
			values[i] = resetValue;
			changed = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0.0f, gap);
		ImGui::SetCursorPosY(rowY);
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragFloat("##value", &values[i], speed, 0.0f, 0.0f, "%.3f"))
			changed = true;
		if (ImGui::IsItemActivated()) BeginEdit();
		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

		ImGui::PopID();
	}

	ImGui::PopID();
	return changed;
}

void EditorLayer::DrawProperties()
{
	ImGui::Begin("Properties");

	if (!mSelectedEntity)
	{
		ImGui::TextDisabled("Select an entity to edit its components.");
		ImGui::End();
		return;
	}

	// Editable entity name.
	char nameBuffer[128];
	const std::string& currentName = mSelectedEntity->GetName();
	const size_t copied = currentName.copy(nameBuffer, sizeof(nameBuffer) - 1);
	nameBuffer[copied] = '\0';

	PropertyLabel("Name");
	if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
		mSelectedEntity->SetName(nameBuffer);
	if (ImGui::IsItemActivated()) BeginEdit();
	if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

	ImGui::Separator();

	// A folder only organizes the hierarchy: it has no transform or components to edit.
	if (mSelectedEntity->IsFolder())
	{
		ImGui::TextDisabled("Folder — groups entities in the hierarchy.");
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const Reference<Transform> transform = mSelectedEntity->GetTransform();

		Vector position = transform->GetPosition();
		float32 positionValues[3] = { position.x, position.y, position.z };
		if (DrawVec3Control("Position", positionValues, 1.0f, 0.0f))
			transform->SetPosition(Vector(positionValues[0], positionValues[1], positionValues[2]));

		Vector rotation = transform->GetRotation();
		float32 rotationValues[3] = { rotation.x, rotation.y, rotation.z };
		if (DrawVec3Control("Rotation", rotationValues, 0.5f, 0.0f))
			transform->SetRotation(Vector(rotationValues[0], rotationValues[1], rotationValues[2]));

		Vector scale = transform->GetScale();
		float32 scaleValues[3] = { scale.x, scale.y, scale.z };
		if (DrawVec3Control("Scale", scaleValues, 0.01f, 1.0f))
			transform->SetScale(Vector(scaleValues[0], scaleValues[1], scaleValues[2]));
	}

	// Draw components in their stored order so a newly added one always appears at the end; headers
	// can be dragged onto each other to reorder. Removal/reorder are deferred to after the loop so
	// the component list is never mutated mid-iteration.
	Component* componentToRemove = nullptr;
	int dragFrom = -1, dragTo = -1;

	const std::vector<Scope<Component>>& components = mSelectedEntity->GetComponents();
	for (int i = 0; i < static_cast<int>(components.size()); ++i)
	{
		Component* component = components[i].get();
		ImGui::PushID(i);
		bool remove = false;

		if (SpriteRenderer* renderer = dynamic_cast<SpriteRenderer*>(component))
		{
			if (DrawComponentHeader("Sprite Renderer", i, remove, dragFrom, dragTo))
			{
				// Reload the texture only once the user finishes editing (not per keystroke).
				char textureBuffer[256];
				const std::string& path = renderer->GetTexturePath();
				const size_t length = path.copy(textureBuffer, sizeof(textureBuffer) - 1);
				textureBuffer[length] = '\0';

				// The field gives up just enough room for the browse button at the end of the row.
				const ImGuiStyle& style = ImGui::GetStyle();
				const float32 browseWidth = ImGui::CalcTextSize("...").x + style.FramePadding.x * 2.0f;

				PropertyLabel("Texture");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseWidth - style.ItemInnerSpacing.x);
				ImGui::InputText("##texture", textureBuffer, sizeof(textureBuffer));
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) { renderer->SetTexturePath(textureBuffer); CommitEdit(); }

				// Drop an image from the Project panel to assign it.
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LN_ASSET_PATH"))
					{
						RecordSnapshot();
						renderer->SetTexturePath(static_cast<const char8*>(payload->Data));
					}

					ImGui::EndDragDropTarget();
				}

				// Browse for a sprite file; store the path relative to the resource root.
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				if (ImGui::Button("..."))
				{
					const std::string picked = FileDialog::Open("Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0");

					if (!picked.empty())
					{
						RecordSnapshot();
						renderer->SetTexturePath(ToResourceRelativePath(picked));
					}
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Browse for a sprite image");
			}
		}
		else if (RigidBody2D* body = dynamic_cast<RigidBody2D*>(component))
		{
			if (DrawComponentHeader("Rigid Body 2D", i, remove, dragFrom, dragTo))
			{
				static const char8* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };

				int32 typeIndex = static_cast<int32>(body->GetBodyType());
				PropertyLabel("Type");
				if (ImGui::Combo("##type", &typeIndex, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
				{
					RecordSnapshot();
					body->SetBodyType(static_cast<BodyType>(typeIndex));
				}

				bool fixedRotation = body->IsFixedRotation();
				PropertyLabel("Fixed Rotation");
				if (ImGui::Checkbox("##fixedRotation", &fixedRotation))
				{
					RecordSnapshot();
					body->SetFixedRotation(fixedRotation);
				}
			}
		}
		else if (BoxCollider2D* collider = dynamic_cast<BoxCollider2D*>(component))
		{
			if (DrawComponentHeader("Box Collider 2D", i, remove, dragFrom, dragTo))
			{
				float32 width = collider->GetWidth();
				PropertyLabel("Width");
				if (ImGui::DragFloat("##width", &width, 1.0f, 0.0f, 10000.0f)) collider->SetWidth(width);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 height = collider->GetHeight();
				PropertyLabel("Height");
				if (ImGui::DragFloat("##height", &height, 1.0f, 0.0f, 10000.0f)) collider->SetHeight(height);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 density = collider->GetDensity();
				PropertyLabel("Density");
				if (ImGui::DragFloat("##density", &density, 0.05f, 0.0f, 100.0f)) collider->SetDensity(density);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 friction = collider->GetFriction();
				PropertyLabel("Friction");
				if (ImGui::DragFloat("##friction", &friction, 0.01f, 0.0f, 1.0f)) collider->SetFriction(friction);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 restitution = collider->GetRestitution();
				PropertyLabel("Restitution");
				if (ImGui::DragFloat("##restitution", &restitution, 0.01f, 0.0f, 1.0f)) collider->SetRestitution(restitution);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();
			}
		}
		else if (ScriptComponent* script = dynamic_cast<ScriptComponent*>(component))
		{
			if (DrawComponentHeader("Script", i, remove, dragFrom, dragTo))
			{
				// Only scripts compiled into the editor are listed (native C++ registry).
				const std::vector<std::string>& names = ScriptRegistry::GetNames();
				const std::string& bound = script->GetScriptName();

				PropertyLabel("Class");
				if (ImGui::BeginCombo("##class", bound.empty() ? "(none)" : bound.c_str()))
				{
					if (ImGui::Selectable("(none)", bound.empty()))
					{
						RecordSnapshot();
						script->SetScriptName(std::string());
					}

					for (const std::string& name : names)
					{
						if (ImGui::Selectable(name.c_str(), name == bound))
						{
							RecordSnapshot();
							script->SetScriptName(name);
						}
					}

					ImGui::EndCombo();
				}

				if (names.empty())
					ImGui::TextDisabled("No scripts registered in this build.");
			}
		}
		else if (CircleCollider2D* collider = dynamic_cast<CircleCollider2D*>(component))
		{
			if (DrawComponentHeader("Circle Collider 2D", i, remove, dragFrom, dragTo))
			{
				float32 radius = collider->GetRadius();
				PropertyLabel("Radius");
				if (ImGui::DragFloat("##radius", &radius, 1.0f, 0.0f, 10000.0f)) collider->SetRadius(radius);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 density = collider->GetDensity();
				PropertyLabel("Density");
				if (ImGui::DragFloat("##density", &density, 0.05f, 0.0f, 100.0f)) collider->SetDensity(density);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 friction = collider->GetFriction();
				PropertyLabel("Friction");
				if (ImGui::DragFloat("##friction", &friction, 0.01f, 0.0f, 1.0f)) collider->SetFriction(friction);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 restitution = collider->GetRestitution();
				PropertyLabel("Restitution");
				if (ImGui::DragFloat("##restitution", &restitution, 0.01f, 0.0f, 1.0f)) collider->SetRestitution(restitution);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();
			}
		}
		else
		{
			// Any other component — a user-defined one from the game module. The editor has no bespoke
			// UI for it, so it draws a header with the registered type name (empty for an unregistered
			// type) and, for now, a note; per-field editing waits on reflection.
			const std::string& typeName = component->GetTypeName();
			const char8* label = typeName.empty() ? "Component" : typeName.c_str();

			if (DrawComponentHeader(label, i, remove, dragFrom, dragTo))
				ImGui::TextDisabled("Defined in the game module.");
		}

		if (remove)
			componentToRemove = component;

		ImGui::PopID();
	}

	// Apply the deferred reorder, then removal. Order is serialized, so a reorder is an undo step.
	if (dragFrom >= 0 && dragTo >= 0 && dragFrom != dragTo)
	{
		RecordSnapshot();
		mSelectedEntity->MoveComponent(dragFrom, dragTo);
	}

	if (componentToRemove)
	{
		RecordSnapshot();
		mSelectedEntity->RemoveComponent(componentToRemove);
	}

	// "Add Component" lists only the component types the entity doesn't already have.
	ImGui::Separator();

	if (ImGui::Button("Add Component"))
		ImGui::OpenPopup("AddComponentPopup");

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		// New colliders fit the entity's sprite (Unity-style); without one they get a default size.
		// Sizes are unscaled, so the Transform scale applies on top.
		const SpriteRenderer* sprite = mSelectedEntity->GetComponent<SpriteRenderer>();
		const Size spriteSize = sprite ? sprite->GetSize() : Size(100.0f, 100.0f);

		if (!mSelectedEntity->HasComponent<SpriteRenderer>() && ImGui::MenuItem("Sprite Renderer"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<SpriteRenderer>("Sprite/Brickout/tile-1.png");
		}

		if (!mSelectedEntity->HasComponent<RigidBody2D>() && ImGui::MenuItem("Rigid Body 2D"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<RigidBody2D>();
		}

		if (!mSelectedEntity->HasComponent<BoxCollider2D>() && ImGui::MenuItem("Box Collider 2D"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<BoxCollider2D>(spriteSize.width, spriteSize.height);
		}

		if (!mSelectedEntity->HasComponent<CircleCollider2D>() && ImGui::MenuItem("Circle Collider 2D"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<CircleCollider2D>(std::max(spriteSize.width, spriteSize.height) * 0.5f);
		}

		if (!mSelectedEntity->HasComponent<ScriptComponent>() && ImGui::MenuItem("Script"))
		{
			RecordSnapshot();
			mSelectedEntity->AddComponent<ScriptComponent>();
		}

		// Components coming from the loaded game module (user-defined ones). They have no special
		// construction here — created by name, default-constructed, then configured in the Inspector.
		// The built-in types above are listed explicitly, so they are skipped here.
		const auto isAttached = [&](const std::string& name)
		{
			for (const auto& component : mSelectedEntity->GetComponents())
				if (component->GetTypeName() == name) return true;
			return false;
		};

		bool sawGameComponent = false;

		for (const std::string& name : ComponentRegistry::GetNames())
		{
			if (IsBuiltInComponent(name) || isAttached(name))
				continue;

			if (!sawGameComponent)
			{
				ImGui::Separator();
				sawGameComponent = true;
			}

			if (ImGui::MenuItem(name.c_str()))
			{
				RecordSnapshot();
				mSelectedEntity->AddComponentByName(name);
			}
		}

		// Scaffold a brand new component class into the game's source tree, Unreal-style.
		ImGui::Separator();

		if (ImGui::MenuItem("New C++ Component..."))
			mOpenNewComponentPopup = true;

		ImGui::EndPopup();
	}

	ImGui::End();
}

void EditorLayer::DrawMenuBar()
{
	static const char8* sceneFilter = "Lion Scene (*.json)\0*.json\0";

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene"))
			{
				RecordSnapshot();
				mScene->Clear();
				mSelectedEntity = nullptr;
			}

			if (ImGui::MenuItem("Open Scene..."))
			{
				const std::string path = FileDialog::Open(sceneFilter);

				if (!path.empty())
				{
					RecordSnapshot();
					mSelectedEntity = nullptr;
					SceneSerializer::Deserialize(mScene, path);
				}
			}

			if (ImGui::MenuItem("Save Scene As..."))
			{
				const std::string path = FileDialog::Save(sceneFilter, "json");

				if (!path.empty())
					SceneSerializer::Serialize(mScene, path);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4"))
				Window::RequestClose();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !mUndoStack.empty()))
				Undo();

			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !mRedoStack.empty()))
				Redo();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Show Colliders", nullptr, &mShowColliders);
			ImGui::MenuItem("ImGui Demo Window", nullptr, &mShowDemo);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Game"))
		{
			if (ImGui::MenuItem("Compile", KeybindToString(mBinds[static_cast<int>(ShortcutAction::CompileModule)]).c_str(), false, !mBuilding))
				CompileGameModule();

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Rebuild the game module and reload it");

			if (ImGui::MenuItem("Reload Module", KeybindToString(mBinds[static_cast<int>(ShortcutAction::ReloadModule)]).c_str(), false, !mBuilding))
				ReloadGameModule();

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Pick up an already-rebuilt Game.dll without restarting the editor");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			DrawLayoutMenu();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Shortcuts", "F1", &mShowShortcuts);
			ImGui::EndMenu();
		}

		// A build runs on a worker thread and takes seconds, so say so where it cannot be missed.
		if (mBuilding)
		{
			static const char8* label = "Compiling the game module...";
			ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemSpacing.x * 2.0f);
			ImGui::TextColored(LogLevelColor(LogLevel::Warning), "%s", label);
		}

		ImGui::EndMainMenuBar();
	}
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceId)
{
	// Three columns: the browsers on the left, the viewport over the console in the middle, and the
	// inspectors on the right. Panel sizes are authored as round pixel counts rather than as
	// free-floating ratios, so at the usual window sizes the layout lands on the same even grid as
	// the rest of the UI.
	constexpr float32 kLeftWidth        = 300.0f;  // Scene Hierarchy over Project.
	constexpr float32 kRightWidth       = 400.0f;  // Statistics over Properties: the inspectors need the extra room.
	constexpr float32 kConsoleHeight    = 256.0f;
	constexpr float32 kProjectHeight    = 324.0f;
	constexpr float32 kStatisticsHeight = 192.0f;

	const ImVec2 work = ImGui::GetMainViewport()->WorkSize;

	// A split ratio is relative to the node being split, and each split shrinks what is left over.
	// Clamping keeps a small window degrading into thinner panels instead of swallowing the viewport.
	const auto ratio = [](float32 size, float32 extent)
	{
		return ImClamp(size / ImMax(extent, 1.0f), 0.05f, 0.5f);
	};

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, work);

	ImGuiID center = dockspaceId;
	ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, ratio(kRightWidth, work.x), nullptr, &center);
	ImGuiID left  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, ratio(kLeftWidth, work.x - kRightWidth), nullptr, &center);

	const ImGuiID bottom     = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, ratio(kConsoleHeight, work.y), nullptr, &center);
	const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, ratio(kProjectHeight, work.y), nullptr, &left);
	const ImGuiID rightTop   = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, ratio(kStatisticsHeight, work.y), nullptr, &right);

	ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
	ImGui::DockBuilderDockWindow("Project", leftBottom);
	ImGui::DockBuilderDockWindow("Viewport", center);
	ImGui::DockBuilderDockWindow("Console", bottom);
	ImGui::DockBuilderDockWindow("Statistics", rightTop);
	ImGui::DockBuilderDockWindow("Properties", right);

	ImGui::DockBuilderFinish(dockspaceId);
}

std::string EditorLayer::LayoutPath(const std::string& name)
{
	return std::string(kLayoutsDirectory) + "/" + name + ".ini";
}

bool EditorLayer::IsValidLayoutName(const std::string& name)
{
	// The name becomes a file name, so keep it to characters that cannot escape the layouts folder.
	if (name.empty() || name.size() > 48)
		return false;

	return std::all_of(name.begin(), name.end(), [](char8 character)
	{
		return std::isalnum(static_cast<unsigned char>(character)) ||
			character == ' ' || character == '-' || character == '_';
	});
}

std::vector<std::string> EditorLayer::SavedLayouts() const
{
	std::vector<std::string> layouts;
	std::error_code error;

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(kLayoutsDirectory, error))
		if (entry.is_regular_file() && entry.path().extension() == ".ini")
			layouts.push_back(entry.path().stem().string());

	std::sort(layouts.begin(), layouts.end());
	return layouts;
}

void EditorLayer::SaveLayout(const std::string& name) const
{
	std::error_code error;
	std::filesystem::create_directories(kLayoutsDirectory, error);

	if (error)
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not create the layouts folder: {}.", error.message()));
		return;
	}

	ImGui::SaveIniSettingsToDisk(LayoutPath(name).c_str());
	Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[Editor] Layout '{}' saved.", name));
}

void EditorLayer::ApplyPendingLayout()
{
	// Rebuilding the dock tree tears down and recreates every node, so it has to happen before any
	// window is submitted this frame. The menu only records the request; this consumes it.
	if (mLayoutRequest == LayoutRequest::None || mDockspaceId == 0)
		return;

	if (mLayoutRequest == LayoutRequest::Reset)
	{
		BuildDefaultLayout(mDockspaceId);
	}
	else if (std::filesystem::exists(LayoutPath(mLayoutToLoad)))
	{
		// Reading an ini rebuilds the dock nodes and re-applies every window's position and size.
		ImGui::LoadIniSettingsFromDisk(LayoutPath(mLayoutToLoad).c_str());
	}
	else
	{
		Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[Editor] Layout '{}' no longer exists.", mLayoutToLoad));
	}

	mLayoutRequest = LayoutRequest::None;
	mLayoutToLoad.clear();
}

void EditorLayer::DrawLayoutMenu()
{
	if (!ImGui::BeginMenu("Layouts"))
		return;

	if (ImGui::MenuItem("Default"))
		mLayoutRequest = LayoutRequest::Reset;

	const std::vector<std::string> layouts = SavedLayouts();

	if (!layouts.empty())
	{
		ImGui::Separator();

		for (const std::string& name : layouts)
			if (ImGui::MenuItem(name.c_str()))
			{
				mLayoutToLoad = name;
				mLayoutRequest = LayoutRequest::Load;
			}
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Save Layout..."))
		mOpenSaveLayoutPopup = true;

	if (ImGui::BeginMenu("Delete Layout", !layouts.empty()))
	{
		for (const std::string& name : layouts)
			if (ImGui::MenuItem(name.c_str()))
			{
				std::error_code error;
				std::filesystem::remove(LayoutPath(name), error);
			}

		ImGui::EndMenu();
	}

	ImGui::EndMenu();
}

void EditorLayer::DrawLayoutPopups()
{
	// The menu item cannot open the popup itself: the menu closes on click, taking the popup's ID
	// scope with it. Opening it here, at the root, keeps the modal alive.
	if (mOpenSaveLayoutPopup)
	{
		mOpenSaveLayoutPopup = false;
		mLayoutName[0] = '\0';
		ImGui::OpenPopup("Save Layout");
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Save Layout", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::TextUnformatted("Layout name");

	if (ImGui::IsWindowAppearing())
		ImGui::SetKeyboardFocusHere();

	ImGui::SetNextItemWidth(256.0f);
	const bool submitted = ImGui::InputText("##name", mLayoutName, IM_ARRAYSIZE(mLayoutName),
		ImGuiInputTextFlags_EnterReturnsTrue);

	const bool valid = IsValidLayoutName(mLayoutName);

	if (mLayoutName[0] != '\0' && !valid)
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "Letters, digits, spaces, - and _ only.");

	ImGui::BeginDisabled(!valid);

	if (ImGui::Button("Save", ImVec2(96.0f, 0.0f)) || (submitted && valid))
	{
		SaveLayout(mLayoutName);
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndDisabled();
	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}

bool EditorLayer::LoadGameModule()
{
	// Both sit next to the editor, alongside the engine's own binaries.
	const std::filesystem::path source = kGameModuleFile;
	const std::filesystem::path runtime = kGameModuleLoadedFile;

	std::error_code error;

	if (!std::filesystem::exists(source, error))
	{
		Log::Console(LogLevel::Warning, "[Editor] No game module next to the editor; only the built-in components are available.");
		return false;
	}

	// Load a *copy*: Windows locks a loaded library, so leaving the original alone is what lets the
	// game module be rebuilt while the editor is still running — the whole point of a reload.
	std::filesystem::copy_file(source, runtime, std::filesystem::copy_options::overwrite_existing, error);

	if (error)
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not copy the game module: {}.", error.message()));
		return false;
	}

	// Bracket the load: the module's static initializers run inside it, and everything they register
	// is attributed to the module so a later reload can drop exactly those entries.
	ComponentRegistry::BeginModule();
	ScriptRegistry::BeginModule();

	const bool loaded = mGameModule.Load(runtime.string());

	ComponentRegistry::EndModule();
	ScriptRegistry::EndModule();

	if (loaded)
		Log::Console(LogLevel::Success, "[Editor] Loaded the game module.");

	return loaded;
}

void EditorLayer::CompileGameModule()
{
	if (mBuilding)
		return;

	const std::filesystem::path root = ProjectRootDirectory();

	if (root.empty())
	{
		Log::Console(LogLevel::Error, "[Editor] Could not locate the project; nothing to compile.");
		return;
	}

	if (MSBuildPath().empty())
	{
		Log::Console(LogLevel::Error, "[Editor] Could not locate MSBuild; is Visual Studio installed?");
		return;
	}

	// Regenerate the projects first: a file that was just scaffolded is not in the .vcxproj yet, and
	// premake globs the source tree. It runs from the project root, where the workspace script lives.
	const std::string generate = "cd /d \"" + root.string() + "\" && premake5 vs2022";

	// Build only the game module. Its MSBuild target is its solution folder and project name — premake
	// files it under the "Runtime" group, hence "Runtime\Game", so moving the project between groups
	// renames this. Building the solution outright would try to relink the running editor. Nothing
	// else is rebuilt, so the editor's copy of the module — which the module's own post-build step
	// refreshes — is the one the reload then picks up.
	const std::string build =
		"\"" + MSBuildPath() + "\""
		" \"" + (root / "Lion.sln").string() + "\""
		" -t:Runtime\\Game"
		" -p:Configuration=" + BuildConfiguration() +
		" -p:Platform=x64 -v:minimal -nologo";

	Log::Console(LogLevel::Information, "[Editor] Compiling the game module...");

	mBuilding = true;
	mGameBuild = std::async(std::launch::async, [generate, build]
	{
		GameBuild result;

		// A failed regeneration means the build would compile a stale file list, so it stops here.
		result.exitCode = RunCommand(generate, result.output);

		if (result.exitCode == 0)
			result.exitCode = RunCommand(build, result.output);
		else
			result.output += "[Editor] Could not regenerate the projects; is premake5 on your PATH?\n";

		return result;
	});
}

void EditorLayer::PollGameBuild()
{
	if (!mBuilding || !mGameBuild.valid())
		return;

	if (mGameBuild.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	const GameBuild result = mGameBuild.get();
	mBuilding = false;

	// The compiler's own diagnostics are the useful part, so they go to the console verbatim.
	std::istringstream lines(result.output);
	std::string line;

	while (std::getline(lines, line))
	{
		while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
			line.pop_back();

		if (line.empty())
			continue;

		const bool failed = line.find("error") != std::string::npos;
		Log::Console(failed ? LogLevel::Error : LogLevel::Information, "[MSBuild] " + line);
	}

	if (result.exitCode != 0)
	{
		Log::Console(LogLevel::Error, "[Editor] The game module failed to compile; the loaded one is unchanged.");
		return;
	}

	// Only now, back on the main thread, is it safe to swap the module out.
	ReloadGameModule();
}

void EditorLayer::ReloadGameModule()
{
	// The simulation holds live instances of the module's types; wind it down first.
	if (mPlaying)
		StopPlay();

	// Every component the module defines is about to lose the code behind its vtable, so the scene
	// makes a round trip through its serialized form: those components are dropped now and recreated
	// by name from the freshly loaded module. Undo/redo and the clipboard are plain text already.
	const std::string scene = SceneSerializer::SerializeToString(mScene);
	const int32 selected = SelectedEntityIndex();

	// Nothing may outlive the module holding a component of its own: an entity kept alive by a stray
	// reference would run its components' destructors after the code was gone.
	mSelectedEntity = nullptr;
	mRenamingEntity = nullptr;
	mEntityToDelete = nullptr;
	mReparentChild = nullptr;
	mReparentTarget = nullptr;
	mEntityLookup.clear();
	mScene->Clear();

	// The registry's factories point into the module, so they go before it does.
	ComponentRegistry::UnloadModule();
	ScriptRegistry::UnloadModule();
	mGameModule.Unload();

	const bool loaded = LoadGameModule();

	// Restore the scene either way: without the module its components are simply skipped, which beats
	// throwing the scene away because a rebuild was broken.
	SceneSerializer::DeserializeFromString(mScene, scene);
	SelectEntityByIndex(selected);

	if (loaded)
		Log::Console(LogLevel::Success, "[Editor] Reloaded the game module.");
}

std::vector<std::string> EditorLayer::ComponentBaseNames()
{
	// Any component class can be a parent: the engine's own (all reachable through the Lion umbrella
	// header) and any the game already defines. "Component" heads the list as the usual choice.
	std::vector<std::string> bases;
	bases.reserve(ComponentRegistry::GetNames().size() + 1);
	bases.emplace_back("Component");

	for (const std::string& name : ComponentRegistry::GetNames())
		bases.push_back(name);

	return bases;
}

bool EditorLayer::GenerateComponent(const std::string& name, const std::string& base)
{
	const std::filesystem::path directory = GameComponentDirectory();

	if (directory.empty())
	{
		Log::Console(LogLevel::Error, "[Editor] Could not locate the game's source tree; is the project checked out?");
		return false;
	}

	std::error_code error;
	std::filesystem::create_directories(directory, error);

	if (error)
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not create '{}': {}.", directory.generic_string(), error.message()));
		return false;
	}

	const std::filesystem::path headerPath = directory / (name + ".h");
	const std::filesystem::path sourcePath = directory / (name + ".cpp");

	if (std::filesystem::exists(headerPath, error) || std::filesystem::exists(sourcePath, error))
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] A component named '{}' already exists.", name));
		return false;
	}

	// An engine base is reachable through the umbrella header and lives in the Lion namespace; a base
	// the game defines sits next to the new file and is unqualified.
	const bool engineBase = (base == "Component") || IsBuiltInComponent(base);
	const std::string qualifiedBase = engineBase ? ("Lion::" + base) : base;
	const std::string baseInclude = engineBase ? std::string() : ("#include \"" + base + ".h\"\n");

	std::ofstream header(headerPath);
	std::ofstream source(sourcePath);

	if (!header.is_open() || !source.is_open())
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not write '{}'.", directory.generic_string()));
		return false;
	}

	header
		<< "#pragma once\n\n"
		<< "#include <Lion/Lion.h>\n"
		<< "#include <Lion/Logic/Serializer.h>\n"
		<< baseInclude
		<< "\n"
		<< "// Component defined by the game. Being compiled into the game module is all it takes for the\n"
		<< "// editor to list it under Add Component: loading the module registers it with the engine.\n"
		<< "//\n"
		<< "// Override the lifecycle hooks to give it behaviour, and archive any field that should survive\n"
		<< "// save/load through Serialize/Deserialize.\n"
		<< "class " << name << " : public " << qualifiedBase << "\n"
		<< "{\n"
		<< "public:\n"
		<< "\tvoid OnAwake() override;\n"
		<< "\tvoid OnUpdate() override;\n\n"
		<< "\tvoid Serialize(Lion::Serializer& serializer) const override;\n"
		<< "\tvoid Deserialize(const Lion::Serializer& serializer) override;\n"
		<< "};\n";

	source
		<< "#include \"" << name << ".h\"\n\n"
		<< "#include <Lion/Logic/ComponentRegistry.h>\n\n"
		<< "using namespace Lion;\n\n"
		<< "void " << name << "::OnAwake()\n"
		<< "{\n"
		<< "\t" << base << "::OnAwake();\n"
		<< "}\n\n"
		<< "void " << name << "::OnUpdate()\n"
		<< "{\n"
		<< "\t" << base << "::OnUpdate();\n"
		<< "}\n\n"
		<< "void " << name << "::Serialize(Serializer& serializer) const\n"
		<< "{\n"
		<< "\t" << base << "::Serialize(serializer);\n"
		<< "}\n\n"
		<< "void " << name << "::Deserialize(const Serializer& serializer)\n"
		<< "{\n"
		<< "\t" << base << "::Deserialize(serializer);\n"
		<< "}\n\n"
		<< "// Binds the class to its name, so scenes can reference it and the editor can list it.\n"
		<< "LION_REGISTER_COMPONENT(" << name << ")\n";

	header.close();
	source.close();

	Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[Editor] Created '{}' in {}.", name, directory.generic_string()));

	// Compile straight away: the class is only usable once it is in the module, and making the user
	// go and build it by hand is the step this whole flow exists to remove.
	CompileGameModule();
	return true;
}

void EditorLayer::DrawNewComponentPopup()
{
	// Opened here, at the root: the Add Component popup closes on click, taking its ID scope with it.
	if (mOpenNewComponentPopup)
	{
		mOpenNewComponentPopup = false;
		mNewComponentName[0] = '\0';
		ImGui::OpenPopup("New C++ Component");
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("New C++ Component", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	const std::vector<std::string> bases = ComponentBaseNames();
	mNewComponentBase = std::clamp(mNewComponentBase, 0, static_cast<int32>(bases.size()) - 1);

	ImGui::TextUnformatted("Parent class");
	ImGui::SetNextItemWidth(320.0f);

	if (ImGui::BeginCombo("##base", bases[mNewComponentBase].c_str()))
	{
		for (int32 i = 0; i < static_cast<int32>(bases.size()); ++i)
			if (ImGui::Selectable(bases[i].c_str(), i == mNewComponentBase))
				mNewComponentBase = i;

		ImGui::EndCombo();
	}

	ImGui::Spacing();
	ImGui::TextUnformatted("Class name");

	if (ImGui::IsWindowAppearing())
		ImGui::SetKeyboardFocusHere();

	ImGui::SetNextItemWidth(320.0f);
	const bool submitted = ImGui::InputText("##name", mNewComponentName, IM_ARRAYSIZE(mNewComponentName),
		ImGuiInputTextFlags_EnterReturnsTrue);

	const std::string name = mNewComponentName;
	const std::filesystem::path directory = GameComponentDirectory();
	const bool valid = IsValidTypeName(name) && !directory.empty() && !ComponentRegistry::Contains(name);

	if (directory.empty())
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "The game's source tree was not found.");
	else if (!name.empty() && !IsValidTypeName(name))
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "Letters, digits and _ only, not starting with a digit.");
	else if (!name.empty() && ComponentRegistry::Contains(name))
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "A component of that name is already registered.");
	else
		ImGui::TextDisabled("%s", (directory / (name.empty() ? "<name>" : name)).generic_string().append(".h/.cpp").c_str());

	ImGui::Spacing();
	ImGui::BeginDisabled(!valid);

	if (ImGui::Button("Create", ImVec2(96.0f, 0.0f)) || (submitted && valid))
	{
		GenerateComponent(name, bases[mNewComponentBase]);
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndDisabled();
	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}
