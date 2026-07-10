#include "EditorLayer.h"

#include "../EditorGui.h"

#include <filesystem>
#include <fstream>

#include <Lion/Core/Filesystem.h>

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
	// Only advance the scene simulation (physics + entity scripts) while playing and not paused.
	if (mPlaying && !mPaused)
		mScene->OnUpdate();
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

	const ImGuiID dockspaceId = ImGui::GetID("LionEditorDockspace");

	if (!mLayoutInitialized)
	{
		mLayoutInitialized = true;

		if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
			BuildDefaultLayout(dockspaceId);
	}

	ImGui::DockSpace(dockspaceId);
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
				ImVec2(rowMin.x + 10.0f, rowMin.y + ImGui::GetTextLineHeight() * 0.5f),
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
			{ ShortcutAction::ToggleColliders, "Viewport",  "Toggle collider hitboxes" },
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
	// Config file for user-customized shortcuts, kept next to imgui.ini (the working directory).
	constexpr const char8* kShortcutsFile = "shortcuts.cfg";
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

	// These are available even while a text field is focused.
	if (IsShortcutPressed(ShortcutAction::Play)) StartPlay();
	if (IsShortcutPressed(ShortcutAction::Pause)) TogglePause();
	if (IsShortcutPressed(ShortcutAction::Stop)) StopPlay();
	if (IsShortcutPressed(ShortcutAction::ToggleShortcuts)) mShowShortcuts = !mShowShortcuts;
	if (IsShortcutPressed(ShortcutAction::ToggleColliders)) mShowColliders = !mShowColliders;

	// The actions below are edit-mode only; ignore them while playing or typing in a text field.
	if (mPlaying || ImGui::GetIO().WantTextInput)
		return;

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

	// The bound tool keys pick the active viewport tool (unless typing, dragging or rebinding).
	if (!mPlaying && mRebindingIndex < 0 && !ImGuizmo::IsUsing() && !ImGui::IsAnyItemActive())
	{
		if (IsShortcutPressed(ShortcutAction::ToolSelect))  mTool = Tool::Select;
		if (IsShortcutPressed(ShortcutAction::GizmoMove))   mTool = Tool::Move;
		if (IsShortcutPressed(ShortcutAction::GizmoRotate)) mTool = Tool::Rotate;
		if (IsShortcutPressed(ShortcutAction::GizmoScale))  mTool = Tool::Scale;
	}

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

	// Play / Pause / Stop, centered along the viewport's top edge.
	const char8* pauseLabel = mPaused ? "Resume" : "Pause";
	const float32 playWidth = ImGui::CalcTextSize("Play").x + style.FramePadding.x * 2.0f;
	const float32 pauseWidth = ImGui::CalcTextSize(pauseLabel).x + style.FramePadding.x * 2.0f;
	const float32 stopWidth = ImGui::CalcTextSize("Stop").x + style.FramePadding.x * 2.0f;
	const float32 totalWidth = playWidth + pauseWidth + stopWidth + style.ItemSpacing.x * 2.0f;

	ImGui::SetCursorScreenPos(ImVec2(imageMin.x + (imageSize.x - totalWidth) * 0.5f, imageMin.y + 8.0f));

	// Play doubles as "resume" while paused, so it stays enabled in that state.
	ImGui::BeginDisabled(mPlaying && !mPaused);
	if (ImGui::Button("Play"))
		StartPlay();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Run the scene simulation (F5)");

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

	// A compact badge, as tall as the drag field next to it. Its frame padding is zeroed below so the
	// letter is centred in the whole badge instead of inside a rectangle shrunk by the padding
	// (which, being narrower than the glyph, pushed the letter to the left).
	const float32 lineHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
	const ImVec2 buttonSize(lineHeight * 0.7f, lineHeight);

	// Reserve room for the trailing label, then split the rest across the three axes.
	const float32 labelWidth = 70.0f;
	const float32 controlsWidth = ImGui::GetContentRegionAvail().x - labelWidth;
	const float32 axisWidth = (controlsWidth - 2.0f * style.ItemInnerSpacing.x) / 3.0f;
	const float32 dragWidth = (axisWidth - buttonSize.x > 12.0f) ? axisWidth - buttonSize.x : 12.0f;

	for (int32 i = 0; i < 3; ++i)
	{
		if (i > 0)
			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

		ImGui::PushID(i);

		// Colored axis badge: click to reset this component.
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

		ImGui::SameLine(0.0f, 0.0f);
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragFloat("##value", &values[i], speed))
			changed = true;
		if (ImGui::IsItemActivated()) BeginEdit();
		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

		ImGui::PopID();
	}

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
	ImGui::TextUnformatted(label);

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

	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
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

				const ImGuiStyle& style = ImGui::GetStyle();
				const float32 browseWidth = ImGui::CalcTextSize("...").x + style.FramePadding.x * 2.0f;
				const float32 labelWidth = 60.0f;

				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseWidth - labelWidth - style.ItemInnerSpacing.x * 2.0f);
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

				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				ImGui::TextUnformatted("Texture");
			}
		}
		else if (RigidBody2D* body = dynamic_cast<RigidBody2D*>(component))
		{
			if (DrawComponentHeader("Rigid Body 2D", i, remove, dragFrom, dragTo))
			{
				static const char8* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };

				int32 typeIndex = static_cast<int32>(body->GetBodyType());
				if (ImGui::Combo("Type", &typeIndex, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
				{
					RecordSnapshot();
					body->SetBodyType(static_cast<BodyType>(typeIndex));
				}

				bool fixedRotation = body->IsFixedRotation();
				if (ImGui::Checkbox("Fixed Rotation", &fixedRotation))
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
				if (ImGui::DragFloat("Width", &width, 1.0f, 0.0f, 10000.0f)) collider->SetWidth(width);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 height = collider->GetHeight();
				if (ImGui::DragFloat("Height", &height, 1.0f, 0.0f, 10000.0f)) collider->SetHeight(height);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 density = collider->GetDensity();
				if (ImGui::DragFloat("Density", &density, 0.05f, 0.0f, 100.0f)) collider->SetDensity(density);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 friction = collider->GetFriction();
				if (ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 1.0f)) collider->SetFriction(friction);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 restitution = collider->GetRestitution();
				if (ImGui::DragFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f)) collider->SetRestitution(restitution);
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

				if (ImGui::BeginCombo("Class", bound.empty() ? "(none)" : bound.c_str()))
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
				if (ImGui::DragFloat("Radius", &radius, 1.0f, 0.0f, 10000.0f)) collider->SetRadius(radius);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 density = collider->GetDensity();
				if (ImGui::DragFloat("Density", &density, 0.05f, 0.0f, 100.0f)) collider->SetDensity(density);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 friction = collider->GetFriction();
				if (ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 1.0f)) collider->SetFriction(friction);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				float32 restitution = collider->GetRestitution();
				if (ImGui::DragFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f)) collider->SetRestitution(restitution);
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();
			}
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

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Shortcuts", "F1", &mShowShortcuts);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceId)
{
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

	ImGuiID center = dockspaceId;
	ImGuiID right        = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
	const ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
	const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.25f, nullptr, &center);

	// Split the right column so Statistics sits on top of Properties.
	const ImGuiID rightTop = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.30f, nullptr, &right);

	ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
	ImGui::DockBuilderDockWindow("Statistics", rightTop);
	ImGui::DockBuilderDockWindow("Properties", right);
	ImGui::DockBuilderDockWindow("Console", bottom);
	ImGui::DockBuilderDockWindow("Project", bottom);
	ImGui::DockBuilderDockWindow("Viewport", center);

	ImGui::DockBuilderFinish(dockspaceId);
}
