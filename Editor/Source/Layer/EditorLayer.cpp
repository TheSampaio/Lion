#include "EditorPch.h"
#include "EditorLayer.h"

#include "../EditorGui.h"
#include "../Expression.h"
#include "../ProjectBuild.h"
#include "../Projects.h"

#include <IconsMaterialDesignIcons.h>
#include "../ModuleSymbols.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <Lion/Core/Filesystem.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

// For running the compile without a console flashing up (see RunCommand). windowsx.h defines IsMaximized
// and friends as macros, so it stays out; this is Windows.h alone.
#ifdef LN_PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

#include <imgui/imgui_internal.h> // DockBuilder API for the default layout.

using namespace Lion;

// The editor's name, which is not the engine's: the project is called Mane, and this is the face it wears.
static const char8* kEditorName = "Lion's Mane";

// One icon scale for the whole editor, on the 4px grid the engines it takes after use: 16px for the icons
// in toolbars, rows and headers, and 24px for the one that heads a panel. Read from here so a change is made
// once, not hunted through every call site.
static constexpr float32 kIconSize = 16.0f;
static constexpr float32 kIconTitle = 24.0f;

// What a file dialog offers when a scene is opened or saved. A scene is JSON inside and a .lnscene outside:
// what it is made of is the engine's business, and what it is is the project's.
static const char8* kSceneFilter = "Lion Scene (*.lnscene)\0*.lnscene\0";

namespace
{
	// Defined with the rest of the project machinery further down the file; the boot needs it before that
	// point is reached.
	void SetActiveProjectDirectory(const std::filesystem::path& project);
}

// Every panel's window name: the icon it wears on its tab, the name it goes by, and the id a saved layout
// remembers it as. ImGui identifies a window by its name — so the name carries the icon — and everything
// after "###" is the part it hashes, which is why the icon can change without a layout forgetting where the
// panel lives.
static const char8* kViewportWindow   = ICON_MDI_MONITOR "  Viewport###Viewport";
static const char8* kHierarchyWindow  = ICON_MDI_FILE_TREE "  Scene Hierarchy###SceneHierarchy";
static const char8* kPropertiesWindow = ICON_MDI_TUNE "  Properties###Properties";
static const char8* kProjectWindow    = ICON_MDI_FOLDER_MULTIPLE "  Content Browser###ContentBrowser";
static const char8* kConsoleWindow    = ICON_MDI_CONSOLE "  Console###Console";
static const char8* kStatisticsWindow = ICON_MDI_CHART_BAR "  Statistics###Statistics";
static const char8* kShortcutsWindow  = ICON_MDI_KEYBOARD "  Shortcuts###Shortcuts";

// Ordered by how much they are reached for: what you pick, what you edit on it, where its assets come
// from, what the engine has to say, and how it is doing. Alt+N follows this, so the numbers you use
// most are the ones under your fingers.
const EditorLayer::Panel EditorLayer::kPanels[5] = {
	{ kHierarchyWindow, "Scene Hierarchy", &EditorLayer::mShowHierarchy,  ShortcutAction::ToggleHierarchy  },
	{ kPropertiesWindow, "Properties",     &EditorLayer::mShowProperties, ShortcutAction::ToggleProperties },
	{ kProjectWindow,   "Content Browser", &EditorLayer::mShowProject,    ShortcutAction::ToggleProject    },
	{ kConsoleWindow,   "Console",         &EditorLayer::mShowConsole,    ShortcutAction::ToggleConsole    },
	{ kStatisticsWindow, "Statistics",     &EditorLayer::mShowStatistics, ShortcutAction::ToggleStatistics },
};

void EditorLayer::OnAttach()
{
	// The size the editor takes when it is un-maximised — a comfortable windowed default. The editor opens
	// maximised; this is what the restore button drops it back to.
	Window::SetSize(1600, 900);
	Window::SetTitle("Lion Engine");
	Window::SetBackgroundColor(0.10f, 0.10f, 0.11f);
	Window::SetResizable(true);
	Window::SetMaximized(true);

	// The editor draws its own caption, so it asks for the strip Windows would have drawn one into. A game
	// asks for neither of these: its window should look like every other window on the machine.
	Window::SetDarkTitleBar(true);
	Window::SetCustomTitleBar(true, kTitleBarHeight);
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

	// The mark the toast wears. Loaded once, because it is the same picture in every toast there will
	// ever be, and it ships beside the executable like the rest of the engine's own assets.
	mLogo = Texture::Create(kEngineIconFile, TextureFilter::Linear);

	Projects::RegisterFileType();
	LoadRecentProjects();
	LoadRecentScenes();

	// The project this session is about, handed over as the process was started: --project from the
	// Project Manager, or a .lnproject double-clicked in Explorer, which arrives as a bare path whose
	// folder is the project. The editor initialises on it — the built-in on its demo, any other project
	// on a fresh scene with its own module built and hot-swapped in as soon as the build answers. Without
	// one (the --no-project-manager skip, or a bare debugger launch), the built-in demo it is. A scene is
	// not an answer: scenes open inside the editor, the way they do in Unreal and Visual Studio.
	std::filesystem::path requested;

	for (int32 argument = 1; argument < CommandLine::GetCount(); ++argument)
	{
		const std::string& value = CommandLine::Get(argument);

		if (value == "--project" && argument + 1 < CommandLine::GetCount())
		{
			requested = CommandLine::Get(argument + 1);
			break;
		}

		if (value.rfind("--", 0) != 0 && std::filesystem::path(value).extension() == Projects::kFileExtension)
		{
			requested = std::filesystem::path(value).parent_path();
			break;
		}
	}

	if (!requested.empty() && Projects::IsProjectFolder(requested)
		&& requested.lexically_normal() != Projects::DefaultProjectDirectory().lexically_normal())
	{
		SetActiveProjectDirectory(requested);
		RememberRecentProject(requested);

		Log::Console(LogLevel::Success,
			LION_FORMAT_TEXT("[Editor] Opened project '{}'.", Projects::DisplayName(requested)));

		// The project's own module is built against the SDK beside the editor — dev tree or distributed
		// install alike. Without an SDK there is nothing to build with, and nothing worth a warning at
		// every boot: the project opens with the module already on disk.
		if (ProjectBuild::Available())
			CompileGameModule();

		return;
	}

	CreateDemoScene();
}

void EditorLayer::OnUpdate()
{
	PollGameBuild();

	// Advance the scene simulation (physics + entity scripts) while playing and not paused — or for
	// exactly one frame when a step was requested, which is what makes a paused run inspectable.
	//
	// A step advances by a whole fixed frame (1/60s), not by the sliver of real time since the last frame:
	// at several hundred FPS that real slice is a fraction of a physics tick, so a dozen steps used to pass
	// before the world moved once — the cooldown that wasn't. A running game keeps real time.
	if (mPlaying && (!mPaused || mStepFrame))
	{
		mScene->OnUpdate(mStepFrame ? (1.0f / 60.0f) : -1.0f);
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
	// The module goes before the editor does, and for the same reason a reload drops it first: the
	// registries hold factories that are code inside it, and the scene holds components whose vtables
	// are. Left to the process to tear down, those are destroyed after the library has been unmapped —
	// a call into memory that is no longer there, on every close.
	UnloadGameModule();

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
	// A scene rebuilt from its serialized form has new entities, so a multi-selection cannot survive it:
	// what comes back is the one entity that was primary, found again by where it sits.
	SetSelection(nullptr);
	mRenamingEntity = nullptr;

	if (index < 0)
		return;

	int current = 0;
	for (const auto& entity : mScene->GetEntities())
	{
		if (current == index)
		{
			SetSelection(entity);
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
	background->GetTransform()->SetPosition(Vector2(0.0f, 0.0f));
	background->AddComponent<SpriteRenderer>("Sprites/Brickout/background.jpg")->SetOrder(Depth::Back);
	mScene->Add(background);

	// A row of bricks.
	for (int32 i = 0; i < 5; i++)
	{
		auto brick = MakeReference<Entity>();
		brick->SetName("Brick " + std::to_string(i + 1));
		brick->GetTransform()->SetPosition(Vector2(-160.0f + i * 80.0f, 60.0f));
		brick->AddComponent<SpriteRenderer>("Sprites/Brickout/tile-" + std::to_string(i + 1) + ".png")->SetOrder(Depth::Middle);
		mScene->Add(brick);
	}

	// Ball (with physics, so pressing Play drops it under gravity).
	auto ball = MakeReference<Entity>();
	ball->SetName("Ball");
	ball->GetTransform()->SetPosition(Vector2(0.0f, 120.0f));
	ball->AddComponent<SpriteRenderer>("Sprites/Brickout/ball.png")->SetOrder(Depth::Middle);
	ball->AddComponent<RigidBody2D>(BodyType::Dynamic, false);
	ball->AddComponent<CircleCollider2D>(6.0f);
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

glm::vec2 EditorLayer::ViewportToWorld(const ImVec2& screen, const ImVec2& imageMin, const ImVec2& imageSize) const
{
	if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
		return mViewCenter;

	// From the image's own pixels to world units: the centre is the camera, and a pixel is 'zoom' of a
	// world unit. Y climbs on screen and falls in the world, hence the flip.
	const float32 offsetX = (screen.x - imageMin.x) - imageSize.x * 0.5f;
	const float32 offsetY = (screen.y - imageMin.y) - imageSize.y * 0.5f;

	return glm::vec2(mViewCenter.x + offsetX * mViewZoom, mViewCenter.y - offsetY * mViewZoom);
}

void EditorLayer::HandleViewportNavigation(const ImVec2& imageMin, const ImVec2& imageSize, bool hovered)
{
	// A running game frames itself; the editor's view is the editor's, and moving it while the game plays
	// would be arguing with the game's own camera.
	if (mPlaying)
		return;

	const ImGuiIO& io = ImGui::GetIO();

	// Pan: the middle button anywhere over the image, or the left button with space held — the two hands
	// Godot gives it, so a mouse without a middle button is not a mouse that cannot pan.
	const bool panning = ImGui::IsMouseDragging(ImGuiMouseButton_Middle)
		|| (ImGui::IsKeyDown(ImGuiKey_Space) && ImGui::IsMouseDragging(ImGuiMouseButton_Left));

	if (panning && (hovered || ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)))
	{
		const ImVec2 delta = ImGui::GetMouseDragDelta(
			ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ? ImGuiMouseButton_Middle : ImGuiMouseButton_Left);

		// The world keeps up with the cursor: dragging right pulls the view left, which is what "grabbing
		// the scene" means.
		mViewCenter.x -= delta.x * mViewZoom;
		mViewCenter.y += delta.y * mViewZoom;

		ImGui::ResetMouseDragDelta(
			ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ? ImGuiMouseButton_Middle : ImGuiMouseButton_Left);
	}

	// Zoom about the cursor: the world point under the pointer is the one that does not move, which is
	// what makes a wheel feel like it is zooming *there* rather than at the middle of the screen.
	if (hovered && io.MouseWheel != 0.0f)
	{
		const glm::vec2 anchor = ViewportToWorld(io.MousePos, imageMin, imageSize);

		constexpr float32 kStep = 1.1f;
		constexpr float32 kMinimumZoom = 0.02f;
		constexpr float32 kMaximumZoom = 50.0f;

		mViewZoom = ImClamp(mViewZoom * std::pow(kStep, -io.MouseWheel), kMinimumZoom, kMaximumZoom);

		const glm::vec2 after = ViewportToWorld(io.MousePos, imageMin, imageSize);
		mViewCenter += anchor - after;
	}

	// Framing the selection is a rebindable editor shortcut (F by default), handled with the rest of
	// them: it should work with the Hierarchy in hand, not only with the pointer over the image.
}

void EditorLayer::FocusViewportOnSelection()
{
	// What to frame: the selection, or the whole scene when nothing is picked — "show me where I am" is
	// the same question either way.
	const std::vector<Reference<Entity>>& targets = mSelection;

	glm::vec2 minimum(FLT_MAX, FLT_MAX);
	glm::vec2 maximum(-FLT_MAX, -FLT_MAX);
	bool any = false;

	const auto include = [&](const Reference<Entity>& entity)
	{
		const Vector position = entity->GetWorldPosition();
		const Vector scale = entity->GetWorldScale();

		// The box the entity actually occupies — its picture's size times the scale, not the scale on its
		// own, which is 1 for everything and would frame a dot. This is the same reading the selection
		// outline takes, so what F fills the viewport with is what the outline was drawn around.
		float32 width = 64.0f;
		float32 height = 64.0f;

		if (const SpriteRenderer* sprite = entity->GetComponent<SpriteRenderer>())
		{
			const Size size = sprite->GetSize();
			width = size.width;
			height = size.height;
		}
		else if (const BoxCollider2D* box = entity->GetComponent<BoxCollider2D>())
		{
			width = box->GetWidth();
			height = box->GetHeight();
		}
		else if (const CircleCollider2D* circle = entity->GetComponent<CircleCollider2D>())
		{
			width = height = circle->GetRadius() * 2.0f;
		}

		const glm::vec2 extent(
			ImMax(std::abs(width * scale.x) * 0.5f, 1.0f),
			ImMax(std::abs(height * scale.y) * 0.5f, 1.0f));

		minimum = glm::min(minimum, glm::vec2(position.x, position.y) - extent);
		maximum = glm::max(maximum, glm::vec2(position.x, position.y) + extent);
		any = true;
	};

	if (!targets.empty())
	{
		for (const auto& entity : targets)
			include(entity);
	}
	else
	{
		for (const auto& entity : mScene->GetEntities())
			if (entity->IsVisible())
				include(entity);
	}

	if (!any)
	{
		mViewCenter = glm::vec2(0.0f, 0.0f);
		mViewZoom = 1.0f;
		return;
	}

	mViewCenter = (minimum + maximum) * 0.5f;

	// Zoom to fit with room to breathe: the tighter of the two axes decides, so nothing lands off-screen.
	// Never closer than one texel to one pixel, the way Unity frames a small object — a 12-pixel ball
	// magnified to fill the viewport is a wall of pixels, not a look at the ball.
	if (mViewportSize.x > 0.0f && mViewportSize.y > 0.0f)
	{
		constexpr float32 kMargin = 1.6f;
		const glm::vec2 span = glm::max(maximum - minimum, glm::vec2(1.0f, 1.0f)) * kMargin;

		const float32 fit = ImMax(span.x / mViewportSize.x, span.y / mViewportSize.y);
		mViewZoom = ImClamp(ImMax(fit, 1.0f), 0.02f, 50.0f);
	}
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

	// Whose eye the scene is seen through: the game's own Camera2D while it runs, and the editor's view
	// otherwise. One camera object either way — what changes is who tells it where to be.
	const Camera2D* current = nullptr;

	if (mPlaying)
	{
		for (const auto& entity : mScene->GetEntities())
		{
			Camera2D* camera = entity->GetComponent<Camera2D>();

			if (camera && camera->IsEnabled())
			{
				current = camera;
				break;
			}
		}
	}

	if (current)
	{
		const glm::vec2 view = current->GetViewPosition();
		mCamera->SetPosition(glm::vec3(view.x, view.y, 0.0f));
		mCamera->SetZoomLevel(current->GetZoom());
	}
	else
	{
		mCamera->SetPosition(glm::vec3(mViewCenter.x, mViewCenter.y, 0.0f));
		mCamera->SetZoomLevel(mViewZoom);
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

	// A window takes the focus on the frame it appears, so the panels drawn after the viewport steal it
	// on the way in and the editor opens on whichever one happens to be submitted last. Claiming it back
	// one frame later — once they have all appeared — starts the session on the scene, which is what the
	// editor is for.
	if (mFocusViewport)
	{
		mFocusViewport = false;
		ImGui::SetWindowFocus(kViewportWindow);
	}

	DrawTitleBar();
	DrawStatusBar();
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
		mFocusViewport = true;

		if (ImGui::DockBuilderGetNode(mDockspaceId) == nullptr)
			mLayoutRequest = LayoutRequest::Reset;
	}

	// A closable panel would otherwise carry two close buttons: one on its tab, and one the dock node
	// draws at the far right of the same row for whichever tab is selected. Two buttons that do the
	// same thing, a few pixels apart. Keep the one on the tab — it is the one attached to the thing it
	// closes, and where Unreal puts it — and drop the node's.
	ImGui::DockSpace(mDockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoCloseButton);
	ImGui::End();

	// --- Panels -----------------------------------------------------------------------------

	DrawViewport();
	DrawHierarchy();
	DrawProperties();

	DrawStatistics();
	DrawConsole();
	DrawProject();
	DrawShortcuts();
	DrawLayoutPopups();

	DrawNewComponentPopup();
	DrawDeleteAssetPopup();
	DrawProjectManagerPopup();

	// Drawn over everything, because that is what they are: the dim that says the game is running, the
	// size a panel reports while it is being dragged, and the toast that says the module is building.
	DrawPlayModeDim();
	DrawPanelSizeOverlay();
	DrawToast();

	// Commit any in-progress continuous edit (gizmo/slider drag) once the mouse is released.
	if (mHasPending && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		CommitEdit();

	// A panel resize is one undo step, bracketed by the mouse. The layout is snapshotted the instant the
	// button goes down — before any drag has moved a separator — so what lands on the history is the layout
	// as it was, not as it is becoming. DrawPanelSizeOverlay is what notices a panel changing size, so this
	// reads its verdict rather than working the drag out a second time.
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		mLayoutBeforePress = ImGui::SaveIniSettingsToMemory(nullptr);
		mResizedThisPress = false;
	}

	if (!mResizingPanels.empty())
		mResizedThisPress = true;

	if (mResizedThisPress && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		mResizedThisPress = false;

		mUndoStack.push_back({ EditKind::Layout, mLayoutBeforePress });

		if (mUndoStack.size() > kMaxUndo)
			mUndoStack.erase(mUndoStack.begin());

		mRedoStack.clear();
	}
}

void EditorLayer::DrawPlayModeDim()
{
	if (!mPlaying)
		return;

	// Playing, a film settles over each panel and leaves the viewport clear: the game is what you are
	// watching, and the editor steps back without going away. A panel is still readable through it — this
	// is a veil, not a curtain. The title and status bars are the window's own frame, not panels, so they
	// keep their colour; re-theming the whole UI would be the same statement paid for in every widget.
	const ImU32 dim = IM_COL32(0, 0, 0, 90);

	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	const auto veil = [&](const char8* name)
	{
		ImGuiWindow* window = ImGui::FindWindowByName(name);

		if (window && !window->Hidden && window->WasActive)
			drawList->AddRectFilled(window->Pos, ImVec2(window->Pos.x + window->Size.x, window->Pos.y + window->Size.y), dim);
	};

	for (const Panel& panel : kPanels)
		veil(panel.window);

	if (mShowShortcuts)
		veil(kShortcutsWindow);
}

void EditorLayer::DrawPanelSizeOverlay()
{
	// A panel says how big it is while it is being dragged, and says nothing the rest of the time. The
	// size is the one the layout file keeps, so what is read here is what can be typed back there.
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		mResizingPanels.clear();
		mPanelSizes.clear();
		return;
	}

	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	const auto report = [&](const char8* name)
	{
		ImGuiWindow* window = ImGui::FindWindowByName(name);

		if (!window || window->Hidden || !window->WasActive)
			return;

		const auto previous = mPanelSizes.find(name);
		const bool resizing = std::find(mResizingPanels.begin(), mResizingPanels.end(), name) != mResizingPanels.end();

		// A panel is being resized when its size changed while the button is held. Once it has, it keeps
		// reporting until the mouse comes up — a number that flickered off between two drag frames would
		// be worse than no number.
		if (!resizing && previous != mPanelSizes.end() &&
			(previous->second.x != window->Size.x || previous->second.y != window->Size.y))
		{
			mResizingPanels.push_back(name);
		}

		mPanelSizes[name] = window->Size;

		if (!resizing && std::find(mResizingPanels.begin(), mResizingPanels.end(), name) == mResizingPanels.end())
			return;

		char8 text[32];
		std::snprintf(text, sizeof(text), "%d x %d", static_cast<int32>(window->Size.x), static_cast<int32>(window->Size.y));

		const ImVec2 size = ImGui::CalcTextSize(text);
		const ImVec2 center(window->Pos.x + window->Size.x * 0.5f, window->Pos.y + window->Size.y * 0.5f);
		const ImVec2 padding(8.0f, 4.0f);

		drawList->AddRectFilled(
			ImVec2(center.x - size.x * 0.5f - padding.x, center.y - size.y * 0.5f - padding.y),
			ImVec2(center.x + size.x * 0.5f + padding.x, center.y + size.y * 0.5f + padding.y),
			IM_COL32(20, 20, 22, 230), 4.0f);

		drawList->AddText(ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f), IM_COL32_WHITE, text);
	};

	for (const Panel& panel : kPanels)
		report(panel.window);

	report(kViewportWindow);
}

void EditorLayer::PushToast(const std::string& message, bool busy)
{
	Toast toast;
	toast.message = message;
	toast.busy = busy;
	toast.time = static_cast<float32>(ImGui::GetTime());
	toast.id = mNextToastId++;

	mToasts.push_back(toast);
}

void EditorLayer::DismissBusyToasts()
{
	// A toast that was waiting on something has nothing left to wait for. It goes, and what it was waiting
	// for gets a toast of its own — which is why the answer never has to shove the question off the screen.
	mToasts.erase(
		std::remove_if(mToasts.begin(), mToasts.end(), [](const Toast& toast) { return toast.busy; }),
		mToasts.end());
}

void EditorLayer::DrawToast()
{
	if (mToasts.empty())
		return;

	// Every toast is the same card: same width, same height, same corner. They are not messages that happen
	// to be boxed — they are one thing the editor says, and a stack of them has to read as a stack.
	constexpr float32 kWidth = 340.0f;
	constexpr float32 kHeight = 68.0f;
	constexpr float32 kGap = 8.0f;
	constexpr float32 kLifetime = 3.5f;
	constexpr float32 kFade = 0.25f;
	constexpr float32 kSlide = 140.0f;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const float32 now = static_cast<float32>(ImGui::GetTime());

	// The oldest sits at the bottom and the newest arrives above it, so a toast never has to be pushed out
	// to make room for the next one.
	int32 slot = 0;

	for (size_t index = 0; index < mToasts.size(); )
	{
		Toast& toast = mToasts[index];
		const float32 age = now - toast.time;

		float32 alpha = ImMin(1.0f, age / kFade);

		if (!toast.busy)
		{
			if (age > kLifetime)
			{
				mToasts.erase(mToasts.begin() + index);
				continue;
			}

			alpha = ImMin(alpha, (kLifetime - age) / kFade);
		}

		// It comes in from the right, off the edge it settles against: a card that fades in where it lands
		// looks like it was always there and you missed it, and the eye follows a thing that moves.
		const float32 travel = (1.0f - ImMin(1.0f, age / kFade)) * kSlide;

		const ImVec2 corner(
			viewport->WorkPos.x + viewport->WorkSize.x - 16.0f + travel,
			viewport->WorkPos.y + viewport->WorkSize.y - 16.0f - slot * (kHeight + kGap));

		ImGui::SetNextWindowPos(corner, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		ImGui::SetNextWindowSize(ImVec2(kWidth, kHeight));
		ImGui::SetNextWindowBgAlpha(0.95f * alpha);

		constexpr ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar;

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

		// A window is identified by its name, and only by its name — pushing an id around it changes
		// nothing. Two toasts sharing one name are one window that gets drawn twice, and only the last of
		// them is ever seen.
		char8 name[24];
		std::snprintf(name, sizeof(name), "##toast%u", toast.id);

		if (ImGui::Begin(name, nullptr, flags))
		{
			// The engine's own face, with the radial running around it: what is spinning is the thing that
			// is working, which is the idea Unreal's throbber is built on.
			constexpr float32 kMark = 34.0f;
			constexpr float32 kRing = 22.0f;

			const ImVec2 windowMin = ImGui::GetWindowPos();
			const ImVec2 center(windowMin.x + 14.0f + kRing, windowMin.y + kHeight * 0.5f);

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			if (mLogo)
				drawList->AddImage(
					static_cast<ImTextureID>(mLogo->GetNativeHandle()),
					ImVec2(center.x - kMark * 0.5f, center.y - kMark * 0.5f),
					ImVec2(center.x + kMark * 0.5f, center.y + kMark * 0.5f),
					ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
					IM_COL32(255, 255, 255, static_cast<int32>(255 * alpha)));

			if (toast.busy)
			{
				// An arc running around the circle, drawn rather than animated frame by frame: where it is,
				// is a function of the clock, so it costs one arc and no state at all.
				const float32 start = now * 3.5f;

				drawList->PathArcTo(center, kRing, start, start + IM_PI * 1.3f, 32);
				drawList->PathStroke(IM_COL32(232, 122, 42, static_cast<int32>(255 * alpha)), ImDrawFlags_None, 3.0f);
			}

			// The message sits on the middle of the card, beside the mark and inside the ring's room.
			const float32 textLeft = 14.0f + kRing * 2.0f + 14.0f;

			ImGui::SetCursorPos(ImVec2(textLeft, (kHeight - ImGui::GetTextLineHeight()) * 0.5f));
			ImGui::PushTextWrapPos(kWidth - 14.0f);
			ImGui::TextUnformatted(toast.message.c_str());
			ImGui::PopTextWrapPos();
		}

		ImGui::End();
		ImGui::PopStyleVar(3);

		slot++;
		index++;
	}
}

void EditorLayer::DrawStatistics()
{
	if (!mShowStatistics)
		return;

	ImGui::Begin(kStatisticsWindow, &mShowStatistics);

	const ImGuiIO& io = ImGui::GetIO();
	const RenderStats& stats = Renderer::GetStats();

	// A row of two: what it is, and what it is worth. A panel of numbers is read down the left and
	// compared down the right, so the two are columns and not one string.
	const auto row = [](const char8* label, const char8* format, ...)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(label);
		ImGui::TableSetColumnIndex(1);

		va_list arguments;
		va_start(arguments, format);
		ImGui::TextV(format, arguments);
		va_end(arguments);
	};

	// Every group's value column starts at the same x, so the numbers line up down the whole panel and not
	// only within a group. A shared fixed width for the label column is what does it: left to itself, each
	// table sized its own columns to its own longest label, and no two groups agreed. The width is enough
	// for the longest label there is ("Textures alive").
	constexpr float32 kLabelWidth = 132.0f;
	constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchProp;

	const auto beginTable = [&](const char8* id) -> bool
	{
		if (!ImGui::BeginTable(id, 2, tableFlags))
			return false;

		ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelWidth);
		ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	};

	if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen) && beginTable("Frame"))
	{
		row("FPS", "%.1f", io.Framerate);
		row("Frame time", "%.3f ms", 1000.0f / io.Framerate);
		row("Build", "%s", BuildConfiguration());
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen) && beginTable("Renderer"))
	{
		// The batch's whole job is to turn many sprites into few draw calls, and none of that shows from
		// the outside. These are the numbers that say whether it is doing it.
		row("Draw calls", "%u", stats.drawCalls);
		row("Sprites", "%u", stats.sprites);
		row("Texture slots", "%u", stats.textureSlots);
		row("Textures alive", "%u", Texture::GetLiveCount());
		row("Vertices", "%u", stats.vertices);
		row("Indices", "%u", stats.indices);

		if (stats.spritesDropped > 0)
			row("Dropped", "%u (out of texture slots)", stats.spritesDropped);

		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen) && beginTable("Scene"))
	{
		int32 components = 0;
		int32 bodies = 0;

		for (const auto& entity : mScene->GetEntities())
		{
			components += static_cast<int32>(entity->GetComponents().size());

			if (entity->HasComponent<RigidBody2D>())
				bodies++;
		}

		row("Entities", "%d", static_cast<int32>(mScene->GetEntities().size()));
		row("Components", "%d", components);
		row("Rigid bodies", "%d", bodies);
		row("Selected", "%d", static_cast<int32>(mSelection.size()));
		row("State", "%s", mPlaying ? (mPaused ? "Paused" : "Playing") : "Editing");
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen) && beginTable("Viewport"))
	{
		row("Render target", "%.0f x %.0f", mViewportSize.x, mViewportSize.y);
		row("Window", "%d x %d", static_cast<int32>(io.DisplaySize.x), static_cast<int32>(io.DisplaySize.y));
		ImGui::EndTable();
	}

	ImGui::End();
}

namespace
{
	// Defined further down, next to the other icon helpers, but needed here for the console's filter icons.
	void DrawIcon(const ImVec2& origin, const ImVec2& box, const char8* icon, ImU32 color, float32 pixels);

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

	// The icon each severity wears in the console, coloured with the severity's own colour: an alert for a
	// warning, a filled alert for an error, an i for information — the shapes Unity uses, so a glance at the
	// left edge reads the same way it does there.
	const char8* LogLevelIcon(LogLevel level)
	{
		switch (LogLevelBucket(level))
		{
			case LogBucket::Error:   return ICON_MDI_ALERT_CIRCLE;
			case LogBucket::Warning: return ICON_MDI_ALERT;
			default:                 return ICON_MDI_INFORMATION;
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

	// A severity filter: a 16px icon and its message count, toggling the filter on click. The icon is drawn
	// rather than set inline so it is a true 16px square; the count sits beside it.
	bool LogFilterToggle(const char8* icon, int count, bool& enabled, const ImVec4& color)
	{
		constexpr float32 kFilterIcon = kIconSize;

		const std::string countText = std::to_string(count);
		const ImGuiStyle& style = ImGui::GetStyle();
		const float32 width = kFilterIcon + style.ItemInnerSpacing.x + ImGui::CalcTextSize(countText.c_str()).x;

		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const float32 height = ImGui::GetFrameHeight();

		// Each filter needs its own id, or the three buttons all answer to "##filter" and ImGui flags the
		// clash. The icon differs per severity, so it is what tells them apart.
		ImGui::PushID(icon);

		// Lit when the filter is on, flat when it is off, so the row reads as a set of toggles.
		if (enabled)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 0.22f));

		const bool clicked = ImGui::Button("##filter", ImVec2(width + style.FramePadding.x * 2.0f, height));

		if (enabled)
			ImGui::PopStyleColor();

		ImGui::PopID();

		const ImU32 tint = ImGui::GetColorU32(enabled ? color : ImVec4(0.45f, 0.46f, 0.49f, 1.0f));

		DrawIcon(ImVec2(origin.x + style.FramePadding.x, origin.y), ImVec2(kFilterIcon, height), icon, tint, kFilterIcon);

		ImGui::GetWindowDrawList()->AddText(
			ImVec2(origin.x + style.FramePadding.x + kFilterIcon + style.ItemInnerSpacing.x,
				ImFloor(origin.y + (height - ImGui::GetTextLineHeight()) * 0.5f)),
			tint, countText.c_str());

		if (clicked)
			enabled = !enabled;

		return clicked;
	}
}

void EditorLayer::DrawConsole()
{
	if (!mShowConsole)
		return;

	// While the panel is collapsed or sits behind another dock tab there is no layout to build, and
	// leaving the tail counter untouched makes it snap to the newest line once it comes back.
	if (!ImGui::Begin(kConsoleWindow, &mShowConsole))
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
	if (ImGui::InputTextWithHint("##search", ICON_MDI_MAGNIFY "  Search...", filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf)))
		filter.Build();

	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &mConsoleAutoScroll);

	ImGui::SameLine();
	ImGui::Checkbox("Collapse", &mConsoleCollapse);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Show one line per distinct message, with how many times it was logged");

	const std::string errorText = std::string(ICON_MDI_ALERT_CIRCLE "  ") + std::to_string(errorCount);
	const std::string warnText = std::string(ICON_MDI_ALERT "  ") + std::to_string(warningCount);
	const std::string infoText = std::string(ICON_MDI_INFORMATION "  ") + std::to_string(infoCount);
	const float32 togglesWidth =
		ImGui::CalcTextSize(errorText.c_str()).x + ImGui::CalcTextSize(warnText.c_str()).x +
		ImGui::CalcTextSize(infoText.c_str()).x + style.FramePadding.x * 6.0f + style.ItemSpacing.x * 2.0f;

	ImGui::SameLine(ImGui::GetContentRegionMax().x - togglesWidth);
	LogFilterToggle(ICON_MDI_ALERT_CIRCLE, errorCount, mConsoleShowErrors, LogLevelColor(LogLevel::Error));
	ImGui::SameLine();
	LogFilterToggle(ICON_MDI_ALERT, warningCount, mConsoleShowWarnings, LogLevelColor(LogLevel::Warning));
	ImGui::SameLine();
	LogFilterToggle(ICON_MDI_INFORMATION, infoCount, mConsoleShowInfo, LogLevelColor(LogLevel::Information));

	ImGui::Separator();

	// --- Collect the entries that pass the filters; the list below indexes into this.
	//
	// Collapsed, a message that was logged a hundred times is one line that says so. The pass is over the
	// history, which is capped — the same order the filter pass already costs — and the clipper still
	// draws only what is on screen, so the panel does not get more expensive for saying less.
	mConsoleVisible.clear();
	mConsoleCounts.clear();

	std::unordered_map<std::string, int> collapsed;   // Message -> row already emitted for it.

	for (int index = 0; index < static_cast<int>(history.size()); ++index)
	{
		const LogBucket bucket = LogLevelBucket(history[index].level);

		if ((bucket == LogBucket::Error && !mConsoleShowErrors) ||
			(bucket == LogBucket::Warning && !mConsoleShowWarnings) ||
			(bucket == LogBucket::Info && !mConsoleShowInfo))
			continue;

		if (!filter.PassFilter(history[index].message.c_str()))
			continue;

		if (mConsoleCollapse)
		{
			const auto existing = collapsed.find(history[index].message);

			// The row keeps the latest occurrence, so its timestamp answers "when did this last happen".
			if (existing != collapsed.end())
			{
				mConsoleVisible[existing->second] = index;
				mConsoleCounts[existing->second]++;
				continue;
			}

			collapsed.emplace(history[index].message, static_cast<int>(mConsoleVisible.size()));
		}

		mConsoleVisible.push_back(index);
		mConsoleCounts.push_back(1);
	}

	// --- Message list: a coloured severity icon, a dimmed timestamp and the message. The clipper submits
	// only the rows actually on screen, so a full history costs no more than a screenful.
	//
	// The child spans the whole panel, padding and all, so a hovered row reaches its left and right edges the
	// way the Hierarchy's do. The margins the content wants are added to each item by hand, not taken out of
	// the panel — a child inset by the window's padding would hold every hover a few pixels off both sides.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::SetCursorPosX(0.0f);
	ImGui::BeginChild("ConsoleOutput", ImVec2(ImGui::GetWindowWidth(), 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PopStyleVar();

	// Scrolling the wheel up over the console lets go of the tail, so the user can read back through the
	// history freely. Caught here, before the rows are drawn, because it is what decides whether the follow
	// re-pins below. A drag on the scrollbar is caught the same way further down, by noticing the view is no
	// longer at the bottom.
	if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel > 0.0f)
		mConsoleStickToBottom = false;

	// A compact row: a 16px severity icon, the timestamp and the message, on a line no taller than it needs
	// to be — which is how the engines this takes after keep a console dense enough to read a run of logs.
	constexpr float32 kConsoleLeftPad = 8.0f;
	constexpr float32 kConsoleIcon = kIconSize;
	constexpr float32 kConsoleRow = 22.0f;
	const float32 panelWidth = ImGui::GetContentRegionMax().x;

	int contextIndex = -1;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImFont* iconFont = EditorGui::GetIconFont();
	const float32 textHeight = ImGui::GetTextLineHeight();

	// A selected row is the engine's orange, the same as the Hierarchy's — the selection looks like one
	// thing wherever it is.
	const ImVec4 selectAccent = EditorGui::GetAccent();
	ImGui::PushStyleColor(ImGuiCol_Header, selectAccent);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(selectAccent.x, selectAccent.y, selectAccent.z, 0.30f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, selectAccent);

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(mConsoleVisible.size()), kConsoleRow);

	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const int index = mConsoleVisible[row];
			const LogEntry& entry = history[index];
			const ImVec4 accent = LogLevelColor(entry.level);

			ImGui::PushID(index);

			const ImVec2 rowMin = ImGui::GetCursorScreenPos();

			// One selectable the width of the panel and the height of the row: the hit area, the hover and
			// the selection all reach the edges. Everything on top of it is painted, so it does not steal
			// the clicks.
			if (ImGui::Selectable("##row", mConsoleSelected == index, ImGuiSelectableFlags_None, ImVec2(0.0f, kConsoleRow)))
				mConsoleSelected = index;

			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				mConsoleSelected = index;
				contextIndex = index;
			}

			// The severity icon, in the severity's colour, centred down the row.
			if (iconFont)
			{
				const char8* icon = LogLevelIcon(entry.level);
				const ImVec2 glyph = iconFont->CalcTextSizeA(kConsoleIcon, FLT_MAX, 0.0f, icon);
				drawList->AddText(iconFont, kConsoleIcon,
					ImVec2(rowMin.x + kConsoleLeftPad, ImFloor(rowMin.y + (kConsoleRow - glyph.y) * 0.5f)),
					ImGui::GetColorU32(accent), icon);
			}

			// Timestamp and message, painted so they centre in the row rather than sitting at its top.
			const float32 textY = ImFloor(rowMin.y + (kConsoleRow - textHeight) * 0.5f);
			const float32 messageX = rowMin.x + kConsoleLeftPad + kConsoleIcon + 12.0f;

			drawList->AddText(ImVec2(messageX, textY), ImGui::GetColorU32(ImGuiCol_TextDisabled), entry.time.c_str());

			// Only errors and warnings tint their message; everything else reads as plain text.
			const bool tinted = LogLevelBucket(entry.level) != LogBucket::Info;
			const ImU32 messageColor = tinted ? ImGui::GetColorU32(accent) : ImGui::GetColorU32(ImGuiCol_Text);

			drawList->AddText(ImVec2(messageX + 84.0f, textY), messageColor, entry.message.c_str());

			// How many times this one line happened, pinned to the right edge where a count goes.
			if (mConsoleCounts[row] > 1)
			{
				char8 count[16];
				std::snprintf(count, sizeof(count), "%d", mConsoleCounts[row]);

				const ImVec2 size = ImGui::CalcTextSize(count);
				const ImVec2 badge(rowMin.x + panelWidth - size.x - 16.0f, textY);

				drawList->AddRectFilled(
					ImVec2(badge.x - 6.0f, badge.y),
					ImVec2(badge.x + size.x + 6.0f, badge.y + textHeight),
					ImGui::GetColorU32(ImGuiCol_Button), 3.0f);

				drawList->AddText(badge, ImGui::GetColorU32(ImGuiCol_Text), count);
			}

			ImGui::PopID();
		}
	}

	clipper.End();
	ImGui::PopStyleColor(3);

	if (contextIndex >= 0)
		ImGui::OpenPopup("ConsoleRowContext");

	if (ImGui::BeginPopup("ConsoleRowContext"))
	{
		if (ImGui::MenuItem("Copy message") && mConsoleSelected >= 0 && mConsoleSelected < static_cast<int>(history.size()))
			ImGui::SetClipboardText(history[mConsoleSelected].message.c_str());

		ImGui::EndPopup();
	}

	// Stay pinned to the bottom while the view is resting there — which it is when the editor first opens, so
	// the newest line shows in full instead of half-clipped at the edge. New content pins too, so a line
	// logged while the console is at the bottom scrolls it into view. Scroll up and the pinning lets go,
	// because the total count moving is not a reason to yank someone back down.
	// While following, keep the view pinned to the maximum every frame — which lands on the newest line in
	// full (SetScrollHereY lands a row shy after a clipper) and rides out the one-frame lag in the maximum as
	// rows stream in, so the editor opens on the last line rather than a screen above it. The wheel above
	// breaks the follow; scrolling back to the bottom, by wheel or scrollbar, restores it.
	mConsoleLastTotal = Log::GetTotalCount();

	if (mConsoleAutoScroll && mConsoleStickToBottom)
		ImGui::SetScrollY(ImGui::GetScrollMaxY());
	else if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
		mConsoleStickToBottom = true;

	ImGui::EndChild();
	ImGui::End();
}

namespace
{
	// The editor keeps its persisted state (UI layout, shortcuts, saved dock layouts) under Data/,
	// which sits in the resource root. The Project panel skips the whole folder: it is editor state,
	// not an asset.
	constexpr const char8* kDataDirectory = "Data";

	// The editor's state lives beside the executable, not in whatever directory the editor happened to
	// be started from — Visual Studio starts it from the project folder, and a shortcut can point
	// anywhere. The rule lives in Projects.h, shared with the Project Manager window.
	std::filesystem::path EditorDataDirectory()
	{
		return Projects::EditorDataDirectory();
	}

	std::filesystem::path EditorLayoutsDirectory()
	{
		return EditorDataDirectory() / "Layouts";
	}

	// Component types the editor adds through a bespoke Add Component entry, because they need
	// construction arguments (a collider sizes itself to the sprite). Everything else in the registry
	// comes from the game module and is added generically, by name.
	constexpr const char8* kBuiltInComponents[] = {
		"SpriteRenderer", "Camera2D", "RigidBody2D", "BoxCollider2D", "CircleCollider2D" };

	bool IsBuiltInComponent(const std::string& name)
	{
		for (const char8* entry : kBuiltInComponents)
			if (name == entry)
				return true;

		return false;
	}

	// The project machinery is shared with the Project Manager window and lives in Projects.h; these keep
	// the names this file has always spoken in.
	std::filesystem::path ProjectRootDirectory()
	{
		return Projects::EngineRootDirectory();
	}

	bool IsProjectFolder(const std::filesystem::path& folder)
	{
		return Projects::IsProjectFolder(folder);
	}

	std::string ProjectDisplayName(const std::filesystem::path& project)
	{
		return Projects::DisplayName(project);
	}

	// The active project: the folder that holds a game's Assets and Source. It is what the Content Browser
	// browses, what a component is scaffolded into, and what the title bar names. It opens on the built-in
	// Sandbox found beside the editor, and --project or Open Project points it at another folder — which
	// is what makes the editor hold more than one game. The engine root above stays put: it is the engine's
	// solution, shared by every project's module build.
	std::filesystem::path& ActiveProjectStorage()
	{
		static std::filesystem::path project = Projects::DefaultProjectDirectory();
		return project;
	}

	std::filesystem::path ActiveProjectDirectory()
	{
		return ActiveProjectStorage();
	}

	void SetActiveProjectDirectory(const std::filesystem::path& project)
	{
		ActiveProjectStorage() = project;
	}

	// The game's assets, in the active project. Empty when no project is around.
	std::filesystem::path GameAssetsDirectory()
	{
		const std::filesystem::path project = ActiveProjectDirectory();
		return project.empty() ? project : (project / "Assets");
	}

	// The folder a component is generated into, as typed in the New C++ Component popup: a path under
	// the game's assets. A script is one of the game's own files, so it belongs where its sprites and
	// shaders are — in the Project panel, the only place the editor ever shows the game's contents. It
	// is compiled into the module and does not ship beside it (see the asset copy in Sandbox/premake5.lua).
	//
	// Empty when the project is not around, or when the path climbs out of the assets folder: the editor
	// writes where the game keeps its files, and nowhere else.
	std::filesystem::path ComponentDirectory(const std::string& folder)
	{
		const std::filesystem::path assets = GameAssetsDirectory();

		if (assets.empty())
			return {};

		const std::filesystem::path directory = (assets / folder).lexically_normal();
		const std::string relative = directory.lexically_relative(assets).generic_string();

		if (relative.empty() || relative.rfind("..", 0) == 0)
			return {};

		return directory;
	}

	// What the Project panel browses: the game's assets in the project, not the copy the build leaves
	// beside the executable. It is what the user actually edits, and it is the only one that holds the
	// files a build deliberately leaves behind — the scripts. An editor shipped without the project has
	// nothing but its own folder, so that is what it falls back to.
	std::filesystem::path ProjectPanelDirectory()
	{
		const std::filesystem::path assets = GameAssetsDirectory();
		return assets.empty() ? std::filesystem::path(ResourceRootDirectory()) : assets;
	}

	// The symbols follow the module: the editor's copy of the library gets a copy of the PDB and is
	// pointed at it, so a debugger attached to the editor holds *that* file open and the one the linker
	// owns stays writable. Without this, rebuilding the game from the editor fails under the debugger —
	// which is the way the editor is usually run.
	//
	// Symbols are optional (an optimised build has none), so failing here costs debugging, not the load.
	void CopyGameSymbols(const std::filesystem::path& moduleDirectory, const std::filesystem::path& destination)
	{
		const std::filesystem::path symbols = moduleDirectory / kGameModuleSymbolsFile;

		std::error_code error;

		if (!std::filesystem::exists(symbols, error))
			return;

		// The copy sits where the loaded module's copy sits — the writable Data folder — and the module
		// points at it by file name, which a debugger resolves beside the module.
		std::filesystem::copy_file(symbols, destination / kGameModuleLoadedSymbolsFile,
			std::filesystem::copy_options::overwrite_existing, error);

		if (error || !RedirectModuleSymbols((destination / kGameModuleLoadedFile).string(), kGameModuleLoadedSymbolsFile))
			Log::Console(LogLevel::Warning, "[Editor] Could not copy the game module's symbols; rebuilding it while debugging the editor will fail.");
	}

	// Runs a command through cmd.exe, capturing stdout and stderr, and returns its exit code.
	//
	// Not _popen: _popen spawns a console, and a Shipping editor has none of its own to lend the child, so
	// Windows gives each one a window — and a compile is several commands, hence "a bunch of consoles". This
	// creates the process with CREATE_NO_WINDOW and reads a pipe, so nothing flashes in any configuration.
	int32 RunCommand(const std::string& command, std::string& output)
	{
		SECURITY_ATTRIBUTES security = {};
		security.nLength = sizeof(security);
		security.bInheritHandle = TRUE;   // The child inherits the write end; that is how it reaches the pipe.

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;

		if (!CreatePipe(&readPipe, &writePipe, &security, 0))
			return -1;

		// The read end is ours alone: a child that inherited it could keep the pipe open and hang the read.
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOA startup = {};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.hStdOutput = writePipe;
		startup.hStdError = writePipe;   // Both streams down one pipe, in the order they were written.

		// cmd.exe, because the commands use its cd and && — but with no window for it to open. The command
		// is wrapped in one more pair of quotes: cmd /C strips the outer pair, and without them it would
		// mangle a command that opens with a quoted path (the MSBuild one, under "C:\Program Files").
		std::string line = "cmd.exe /C \"" + command + "\"";

		PROCESS_INFORMATION process = {};
		const BOOL started = CreateProcessA(nullptr, line.data(), nullptr, nullptr, TRUE,
			CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);

		// The write end has to be closed here too, or the read below never sees end-of-file: the child is not
		// the only thing holding it open until this does.
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

		return static_cast<int32>(exitCode);
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


	// Inspector rows put the label first, in a fixed column, and the widget after it — ImGui's default
	// is the other way round, which reads backwards for a property. The column is wide enough for the
	// longest label in use ("Fixed Rotation"), so every row in every component lines up.
	constexpr float32 kPropertyLabelWidth = 100.0f;

	// A Transform's rows carry their own, narrower column: their labels are short words, and the width
	// the shared column keeps for "Fixed Rotation" is width three axes need far more — it is the
	// difference between reading three decimals and reading two.
	constexpr float32 kVectorLabelWidth = 64.0f;

	// The padlock and the revert arrow are glyphs, not buttons, so they are sized like glyphs — a square
	// the height of the text rather than the height of a field. What they gave back goes to the fields,
	// which is where it was missing: a Transform's three axes have to fit three decimals.
	//
	// A row stops short by exactly one of these, drawn or not: a column that appears and disappears would
	// drag every field on the row with it.
	// The width a row's end widget claims — the eye, the padlock, the revert arrow. A 16px icon with a little
	// room around it, which is the size those are in Unity and Godot.
	float32 RowEndSlot()
	{
		return 20.0f;
	}

	// The gap before a row-end glyph — tighter than the spacing between fields, because a glyph on its
	// own does not need the room a field does.
	constexpr float32 kRowEndGap = 2.0f;

	// Centres a row-end glyph against the fields beside it: it is shorter than they are, so left on the
	// row's baseline it would sit high.
	void AlignRowEndGlyph()
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImFloor((ImGui::GetFrameHeight() - RowEndSlot()) * 0.5f));
	}

	// Lays out a property row's label and leaves the cursor on the widget. Pass a width for a widget
	// that sizes itself; the default fills the rest of the row, minus the slot the revert arrow keeps.
	// A number field that also takes arithmetic. ImGui parses a typed value with scanf, which reads
	// "2+3" as 2 and stops; this reads what was actually typed — the widget keeps its own text while it
	// is being edited — and, when that text is an expression, replaces the half-read number with the
	// answer once the edit is committed. An ordinary edit never leaves the path it always took.
	//
	// Wrapped around the widget rather than replacing it, so dragging, clamping, formatting and the undo
	// bracket all stay ImGui's and the editor's.
	bool ApplyTypedExpression(ImGuiID id, float32& value)
	{
		// The text lives only while the field is in text-input mode, so it is caught then and read after,
		// when the field has already gone.
		static ImGuiID sTypingId = 0;
		static std::string sTyped;

		if (ImGui::TempInputIsActive(id))
		{
			if (const ImGuiInputTextState* state = ImGui::GetInputTextState(id))
			{
				sTypingId = id;
				sTyped.assign(state->TextA.Data, static_cast<size_t>(state->TextLen));
			}

			return false;
		}

		if (sTypingId != id)
			return false;

		const std::string typed = sTyped;
		sTypingId = 0;
		sTyped.clear();

		if (!Expression::IsExpression(typed))
			return false;

		const std::optional<float32> evaluated = Expression::Evaluate(typed);

		if (!evaluated || *evaluated == value)
			return false;

		value = *evaluated;
		return true;
	}

	void PropertyLabel(const char8* label, float32 widgetWidth = 0.0f)
	{
		const float32 available = ImGui::GetContentRegionAvail().x - kPropertyLabelWidth
			- RowEndSlot() - kRowEndGap;

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine(kPropertyLabelWidth);
		ImGui::SetNextItemWidth((widgetWidth > 0.0f) ? widgetWidth : ImMax(available, 32.0f));
	}

	// Puts the cursor on the slot the row keeps at its end, wherever the widget before it stopped.
	void SameLineRowEnd()
	{
		ImGui::SameLine(ImGui::GetContentRegionMax().x - RowEndSlot());
		AlignRowEndGlyph();
	}

	// Writes one named field on a component the editor knows nothing about.
	//
	// It cannot call a setter, so it walks the component's own description and stops at the field with the
	// name it was given. Everything else it is shown, it leaves alone — which is how an edit made on one
	// selected entity reaches the same field on the others without anybody knowing the type.
	class FieldSetter : public Reflector
	{
	public:
		FieldSetter(const char8* name, float32 value) : mName(name), mKind(Kind::Float), mFloat(value) {}
		FieldSetter(const char8* name, int32 value) : mName(name), mKind(Kind::Int), mInt(value) {}
		FieldSetter(const char8* name, bool value) : mName(name), mKind(Kind::Bool), mBool(value) {}
		FieldSetter(const char8* name, const std::string& value) : mName(name), mKind(Kind::String), mString(value) {}
		FieldSetter(const char8* name, const Vector& value) : mName(name), mKind(Kind::Vector), mVector(value) {}

		void Field(const char8* name, float32& value) override     { if (Matches(name, Kind::Float))  value = mFloat; }
		void Field(const char8* name, int32& value) override       { if (Matches(name, Kind::Int))    value = mInt; }
		void Field(const char8* name, bool& value) override        { if (Matches(name, Kind::Bool))   value = mBool; }
		void Field(const char8* name, std::string& value) override { if (Matches(name, Kind::String)) value = mString; }
		void Field(const char8* name, Vector& value) override      { if (Matches(name, Kind::Vector)) value = mVector; }
		void FieldAsset(const char8* name, std::string& path) override { if (Matches(name, Kind::String)) path = mString; }

	private:
		enum class Kind { Float, Int, Bool, String, Vector };

		bool Matches(const char8* name, Kind kind) const { return kind == mKind && mName == name; }

		std::string mName;
		Kind mKind;

		float32 mFloat = 0.0f;
		int32 mInt = 0;
		bool mBool = false;
		std::string mString;
		Vector mVector;
	};

	bool ContainsNoCase(const std::string& haystack, const std::string& needle)
	{
		const auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
			[](char8 a, char8 b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });

		return it != haystack.end();
	}

	// A node survives the search when it matches — or when anything under it does. Hiding a branch whose
	// child matches would answer "not here" about something that is only nested.
	bool MatchesHierarchyFilter(const Entity* entity, const char8* filter)
	{
		if (filter == nullptr || filter[0] == '\0')
			return true;

		if (ContainsNoCase(entity->GetName(), filter))
			return true;

		for (const Entity* child : entity->GetChildren())
			if (MatchesHierarchyFilter(child, filter))
				return true;

		return false;
	}

	// A menu bar in a rectangle of the caller's choosing.
	//
	// ImGui's own menu bar can only be a window's top edge, and this one has to begin after a logo and sit
	// in the middle of a bar two rows tall. The work is the same either way — a horizontal layout, its own
	// clip rect and its own nav layer — so this is that work, aimed somewhere else.
	bool BeginMenuBarAt(const ImVec2& barMin, const ImVec2& barMax)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		if (window->SkipItems)
			return false;

		ImGui::BeginGroup();
		ImGui::PushID("##menubar");

		ImRect bar(barMin, barMax);
		bar.ClipWith(window->Rect());
		ImGui::PushClipRect(bar.Min, bar.Max, false);

		window->DC.CursorPos = window->DC.CursorMaxPos = barMin;
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.IsSameLine = false;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;

		ImGui::AlignTextToFramePadding();
		return true;
	}

	void EndMenuBarAt()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		if (window->SkipItems)
			return;

		ImGui::PopClipRect();
		ImGui::PopID();

		// The group must not turn into an item of its own: the bar is a place things are drawn in, not a
		// thing that was drawn. ImGui's own EndMenuBar does exactly this.
		ImGui::GetCurrentContext()->GroupStack.back().EmitItem = false;
		ImGui::EndGroup();

		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.IsSameLine = false;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	}

	// Windows' rule: a thing that arrives where its name is taken gets a number, and a second one gets
	// the next. Renaming a whole selection and duplicating one entity are the same problem, so they ask
	// the same question — what is the first free number for this name?
	//
	// The base is stripped of a number it already carries, so duplicating "Ball (1)" gives "Ball (2)"
	// rather than "Ball (1) (1)".
	std::string NumberedName(const Reference<Scene>& scene, const std::string& name)
	{
		std::string base = name;
		const size_t open = base.find_last_of('(');

		if (open != std::string::npos && base.size() > open + 2 && base.back() == ')' && open >= 2 && base[open - 1] == ' ')
		{
			const std::string digits = base.substr(open + 1, base.size() - open - 2);

			if (!digits.empty() && std::all_of(digits.begin(), digits.end(), [](unsigned char c) { return std::isdigit(c); }))
				base.erase(open - 1);
		}

		const auto taken = [&scene](const std::string& candidate)
		{
			for (const auto& entity : scene->GetEntities())
				if (entity->GetName() == candidate)
					return true;

			return false;
		};

		for (int32 number = 1; ; ++number)
		{
			const std::string candidate = base + " (" + std::to_string(number) + ")";

			if (!taken(candidate))
				return candidate;
		}
	}

	// An icon of an exact pixel size, centred in a box of the caller's choosing. It draws from the standalone
	// icon font — baked at atlas resolution — so 20, 24 or 32 pixels are all sharp, rather than one small
	// baked size scaled up into a blur. This is what every hand-placed icon in the editor goes through.
	void DrawIcon(const ImVec2& origin, const ImVec2& box, const char8* icon, ImU32 color, float32 pixels)
	{
		ImFont* font = EditorGui::GetIconFont();

		if (!font)
			return;

		const ImVec2 glyph = font->CalcTextSizeA(pixels, FLT_MAX, 0.0f, icon);

		ImGui::GetWindowDrawList()->AddText(font, pixels,
			ImVec2(ImFloor(origin.x + (box.x - glyph.x) * 0.5f), ImFloor(origin.y + (box.y - glyph.y) * 0.5f)),
			color, icon);
	}

	// The square variant, for the many callers whose box is a square.
	void DrawIcon(const ImVec2& origin, float32 box, const char8* icon, ImU32 color, float32 pixels)
	{
		DrawIcon(origin, ImVec2(box, box), icon, color, pixels);
	}

	// An icon drawn inline, as if it were a word, at an exact pixel size — larger than the merged inline
	// glyphs. It claims its own space and leaves the cursor after it, so a SameLine puts the next item beside
	// it, which is how the Inspector sets a large mark before an entity's name.
	void DrawInlineIcon(const char8* icon, float32 pixels, ImU32 color)
	{
		ImFont* font = EditorGui::GetIconFont();
		const ImVec2 glyph = font ? font->CalcTextSizeA(pixels, FLT_MAX, 0.0f, icon) : ImVec2(pixels, pixels);
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const float32 line = ImMax(ImGui::GetFrameHeight(), glyph.y);

		if (font)
			ImGui::GetWindowDrawList()->AddText(font, pixels,
				ImVec2(origin.x, ImFloor(origin.y + (line - glyph.y) * 0.5f)), color, icon);

		ImGui::Dummy(ImVec2(glyph.x, line));
	}

	// The eye in the Hierarchy's Visibility column. Open, the entity is drawn; struck through, it is not
	// — and that is all it is: a hidden entity still updates and still collides.
	bool EyeButton(const char8* id, bool visible)
	{
		const float32 size = RowEndSlot();
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
		const bool hovered = ImGui::IsItemHovered();

		if (hovered)
			ImGui::SetTooltip(visible ? "Hide" : "Show");

		const ImU32 color = ImGui::GetColorU32((visible || hovered) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

		DrawIcon(origin, size, visible ? ICON_MDI_EYE : ICON_MDI_EYE_OFF, color, kIconSize);
		return clicked;
	}

	// The revert arrow, as Unreal draws it: a curved arrow at the end of a row, shown only while the field
	// is not what it ships as. Clicking it puts the default back. The slot is held even when nothing is in
	// it — see RowEndSlot — so the fields do not shift as one of them is edited.
	bool ResetToDefaultButton(const char8* id, bool modified)
	{
		const float32 size = RowEndSlot();

		if (!modified)
		{
			ImGui::Dummy(ImVec2(size, size));
			return false;
		}

		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
		const bool hovered = ImGui::IsItemHovered();

		if (hovered)
			ImGui::SetTooltip("Reset to default");

		const ImU32 color = hovered ? IM_COL32(255, 216, 122, 255) : IM_COL32(224, 176, 62, 255);

		DrawIcon(origin, size, ICON_MDI_RESTORE, color, kIconSize);
		return clicked;
	}

	// The uniform-scale padlock: closed, the three axes move together. It says which state it is in rather
	// than which state it would switch to, because a toggle that shows the other state is a toggle nobody
	// trusts.
	bool LockButton(const char8* id, bool locked)
	{
		const float32 size = RowEndSlot();
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
		const bool hovered = ImGui::IsItemHovered();

		if (hovered)
			ImGui::SetTooltip(locked ? "Scale the axes together" : "Scale each axis on its own");

		const ImU32 color = ImGui::GetColorU32((locked || hovered) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

		DrawIcon(origin, size, locked ? ICON_MDI_LOCK : ICON_MDI_LOCK_OPEN_VARIANT, color, kIconSize);
		return clicked;
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

	// What the engine ships into a project, and what a project cannot be edited into not having.
	//
	// The two folders are where an asset of a kind goes: deleting one is not editing a project, it is
	// breaking it. Lit.glsl is the shader everything is drawn with. And Sprites/Geometries is sealed —
	// the simple shapes the engine will put there to build with are the engine's, all the way down, so
	// what is inside it is as protected as the folder around it.
	constexpr const char8* kEngineAssets[] = { "Shaders", "Shaders/Lit.glsl", "Sprites", "Sprites/Geometries" };
	constexpr const char8* kSealedAssetFolders[] = { "Sprites/Geometries" };

	bool IsEngineAsset(const std::string& relative)
	{
		for (const char8* entry : kEngineAssets)
			if (relative == entry)
				return true;

		for (const char8* folder : kSealedAssetFolders)
			if (relative.rfind(std::string(folder) + "/", 0) == 0)
				return true;

		return false;
	}

	// A name the filesystem will take and the project can live with: no separators, and nothing that
	// climbs out of the folder it was typed in.
	bool IsValidAssetName(const std::string& name)
	{
		if (name.empty() || name == "." || name == "..")
			return false;

		return name.find_first_of("/\\:*?\"<>|") == std::string::npos;
	}

	// A folder that does not exist yet, numbered the way Windows numbers one.
	std::filesystem::path UnusedFolderPath(const std::filesystem::path& parent)
	{
		std::error_code error;

		if (!std::filesystem::exists(parent / "New Folder", error))
			return parent / "New Folder";

		for (int32 number = 1; ; ++number)
		{
			const std::filesystem::path candidate = parent / ("New Folder (" + std::to_string(number) + ")");

			if (!std::filesystem::exists(candidate, error))
				return candidate;
		}
	}

	// Only asset-like files are listed; the resource root also holds the executable and its DLLs.
	// A script is one of the game's files too, so it is listed like any other: the Content Browser is
	// where a component is created, and hiding the result would be a strange way to end that flow.
	bool IsAssetFile(const std::filesystem::path& path)
	{
		static const char8* extensions[] = { ".png", ".jpg", ".jpeg", ".bmp", ".glsl", ".lnscene", ".h", ".cpp" };

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
	if (!mShowProject)
		return;

	ImGui::Begin(kProjectWindow, &mShowProject);

	const std::filesystem::path root = ProjectPanelDirectory();

	if (root.empty())
	{
		ImGui::TextDisabled("Could not locate the resource root.");
		ImGui::End();
		return;
	}

	const std::filesystem::path current = root / mProjectPath;

	// A folder change is decided here but applied at the very end, so `current` and the listing stay in
	// step with `mProjectPath` for the whole frame. Mutating `mProjectPath` mid-draw would leave `current`
	// (read once, above) pointing at the folder we just left, and the scan below would read that one while
	// recording it under the new path — so the view would only right itself on the next timer poll.
	bool navigate = false;
	std::string navigateTarget;

	// --- Breadcrumb toolbar.
	ImGui::BeginDisabled(mProjectPath.empty());
	if (ImGui::Button(ICON_MDI_ARROW_UP))
	{
		navigate = true;
		navigateTarget = std::filesystem::path(mProjectPath).parent_path().generic_string();
	}
	ImGui::EndDisabled();

	// Every ancestor is one click away: the root, then each folder on the way down, the last being where we
	// stand. It is the reliable way back — a target you aim at, not a button you might miss.
	ImGui::SameLine();
	if (ImGui::SmallButton("Assets"))
	{
		navigate = true;
		navigateTarget.clear();
	}

	int32 crumbId = 0;
	for (size_t start = 0; start < mProjectPath.size(); )
	{
		const size_t slash = mProjectPath.find('/', start);
		const size_t end = (slash == std::string::npos) ? mProjectPath.size() : slash;
		const std::string segment = mProjectPath.substr(start, end - start);

		if (!segment.empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("/");
			ImGui::SameLine();

			ImGui::PushID(crumbId++);
			if (ImGui::SmallButton(segment.c_str()))
			{
				navigate = true;
				navigateTarget = mProjectPath.substr(0, end);
			}
			ImGui::PopID();
		}

		start = end + 1;
	}

	ImGui::Separator();

	std::error_code error;

	if (!std::filesystem::is_directory(current, error))
	{
		ImGui::TextDisabled("Folder not found; returning to the root.");
		mProjectPath.clear();
		ImGui::End();
		return;
	}

	// --- The listing, read when it changed and not when it is drawn.
	//
	// A folder's own timestamp moves when an entry is added to or removed from it, so one cheap look at it
	// answers "did anything change?" without walking anything. It is looked at on a timer rather than every
	// frame, because a folder somebody else edits is not a folder that changes 600 times a second.
	constexpr double kPollInterval = 0.5;
	const double now = ImGui::GetTime();

	if (mProjectDirty || mProjectScannedPath != mProjectPath)
	{
		ScanProjectDirectory(current);
	}
	else if (now - mProjectPollTime >= kPollInterval)
	{
		mProjectPollTime = now;

		const std::filesystem::file_time_type stamp = std::filesystem::last_write_time(current, error);

		if (!error && stamp != mProjectStamp)
			ScanProjectDirectory(current);
	}

	for (const AssetEntry& entry : mProjectEntries)
	{
		ImGui::PushID(entry.path.c_str());

		if (entry.directory)
		{
			if (DrawAssetEntry(entry.name, entry.path, true) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				navigate = true;
				navigateTarget = entry.path;
			}
		}
		else
		{
			DrawAssetEntry(entry.name, entry.path, false);

			// Drag an asset onto a field that accepts it (e.g. the Sprite Renderer's texture).
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("LN_ASSET_PATH", entry.path.c_str(), entry.path.size() + 1);
				ImGui::TextUnformatted(entry.name.c_str());
				ImGui::EndDragDropSource();
			}
		}

		ImGui::PopID();
	}

	// Right-clicking the empty space is about the folder being browsed, not about anything in it.
	if (ImGui::BeginPopupContextWindow("ContentBrowserContext",
		ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("New Folder"))
			CreateAssetFolder();

		// The component lands where you are standing: the popup opens with this folder already in it.
		if (ImGui::MenuItem("New C++ Component..."))
		{
			const std::string folder = mProjectPath.empty() ? std::string("Scripts") : mProjectPath;
			const size_t copied = folder.copy(mNewComponentFolder, sizeof(mNewComponentFolder) - 1);
			mNewComponentFolder[copied] = '\0';
			mOpenNewComponentPopup = true;
		}

		ImGui::EndPopup();
	}

	if (navigate)
		mProjectPath = navigateTarget;

	ImGui::End();
}

namespace
{
	// The face of a file, by what it holds. A scene is the engine's own kind, so it gets the engine's shape.
	const char8* AssetIcon(const std::string& name)
	{
		const std::string extension = std::filesystem::path(name).extension().string();

		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp")
			return ICON_MDI_IMAGE_OUTLINE;

		if (extension == ".h" || extension == ".cpp")
			return ICON_MDI_CODE_TAGS;

		if (extension == ".glsl")
			return ICON_MDI_PALETTE;

		if (extension == ".lnscene")
			return ICON_MDI_SHAPE;

		return ICON_MDI_FILE_DOCUMENT_OUTLINE;
	}
}

bool EditorLayer::DrawAssetEntry(const std::string& name, const std::string& assetPath, bool folder)
{
	const bool engineOwned = IsEngineAsset(assetPath);

	// Renaming happens where the name is, so the row becomes the field.
	if (mRenamingAsset == assetPath)
	{
		if (mAssetRenameFocus)
		{
			ImGui::SetKeyboardFocusHere();
			mAssetRenameFocus = false;
		}

		ImGui::SetNextItemWidth(-1.0f);
		const bool committed = ImGui::InputText("##rename", mAssetRenameBuffer, sizeof(mAssetRenameBuffer),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

		if (committed || ImGui::IsItemDeactivated())
		{
			RenameAsset(assetPath, mAssetRenameBuffer);
			mRenamingAsset.clear();
		}

		return false;
	}

	// The icon says what the row is, which is what the yellow used to be doing badly: a colour has to be
	// learned, and a folder does not. A folder reads as a normal folder whoever owns it — the engine's
	// Shaders and Sprites are folders like any other; only an engine-owned *file* is dimmed, and that only
	// hints at what the context menu and the tooltip say plainly: it cannot be renamed or deleted.
	const bool dimmed = engineOwned && !folder;

	if (dimmed)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

	// Hovered and clicked in the engine's orange, the same as the Hierarchy and the console: one selection
	// colour across the editor.
	const ImVec4 accent = EditorGui::GetAccent();
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(accent.x, accent.y, accent.z, 0.30f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, accent);

	const std::string label = std::string(folder ? ICON_MDI_FOLDER : AssetIcon(name)) + "  " + name;
	const bool activated = ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);

	ImGui::PopStyleColor(2);

	if (dimmed)
		ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(engineOwned ? "%s\nShipped with the engine. It cannot be renamed or deleted." : "%s",
			assetPath.c_str());

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Copy path"))
			ImGui::SetClipboardText(assetPath.c_str());

		ImGui::Separator();

		// What the engine ships is listed, browsed and used like anything else — it just cannot be taken
		// away. The items are shown rather than hidden, because a menu that is missing the entry someone
		// is looking for teaches them nothing.
		ImGui::BeginDisabled(engineOwned);

		if (ImGui::MenuItem("Rename"))
		{
			mRenamingAsset = assetPath;
			mAssetRenameFocus = true;

			const size_t copied = name.copy(mAssetRenameBuffer, sizeof(mAssetRenameBuffer) - 1);
			mAssetRenameBuffer[copied] = '\0';
		}

		if (ImGui::MenuItem("Delete"))
			mAssetToDelete = assetPath;

		ImGui::EndDisabled();

		if (engineOwned)
			ImGui::TextDisabled("Shipped with the engine.");

		ImGui::EndPopup();
	}

	return activated;
}

void EditorLayer::ScanProjectDirectory(const std::filesystem::path& directory)
{
	mProjectEntries.clear();
	mProjectScannedPath = mProjectPath;
	mProjectDirty = false;

	std::error_code error;
	mProjectStamp = std::filesystem::last_write_time(directory, error);
	mProjectPollTime = ImGui::GetTime();

	// Directories first, then the files: a folder is a place you go, and the places come before the things.
	for (const auto& entry : std::filesystem::directory_iterator(directory, error))
	{
		const std::string name = entry.path().filename().generic_string();

		// The editor's own Data folder sits in the resource root, but it holds no assets.
		if (entry.is_directory() && !(mProjectPath.empty() && name == kDataDirectory))
			mProjectEntries.push_back({ name, mProjectPath.empty() ? name : mProjectPath + "/" + name, true });
	}

	for (const auto& entry : std::filesystem::directory_iterator(directory, error))
	{
		if (entry.is_directory() || !IsAssetFile(entry.path()))
			continue;

		const std::string name = entry.path().filename().generic_string();
		mProjectEntries.push_back({ name, mProjectPath.empty() ? name : mProjectPath + "/" + name, false });
	}
}

void EditorLayer::CreateAssetFolder()
{
	const std::filesystem::path parent = ProjectPanelDirectory() / mProjectPath;
	const std::filesystem::path created = UnusedFolderPath(parent);

	std::error_code error;
	std::filesystem::create_directory(created, error);

	if (error)
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not create the folder: {}.", error.message()));
		return;
	}

	// Straight into a rename, the way every file browser does it: a folder called "New Folder" is a
	// folder nobody meant to keep.
	mProjectDirty = true;

	const std::string name = created.filename().generic_string();
	mRenamingAsset = mProjectPath.empty() ? name : mProjectPath + "/" + name;
	mAssetRenameFocus = true;

	const size_t copied = name.copy(mAssetRenameBuffer, sizeof(mAssetRenameBuffer) - 1);
	mAssetRenameBuffer[copied] = '\0';
}

void EditorLayer::RenameAsset(const std::string& assetPath, const std::string& name)
{
	if (IsEngineAsset(assetPath) || !IsValidAssetName(name))
		return;

	const std::filesystem::path root = ProjectPanelDirectory();
	const std::filesystem::path from = root / assetPath;
	const std::filesystem::path to = from.parent_path() / name;

	if (from == to)
		return;

	std::error_code error;

	if (std::filesystem::exists(to, error))
	{
		Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[Editor] '{}' already exists.", name));
		return;
	}

	std::filesystem::rename(from, to, error);
	mProjectDirty = true;

	if (error)
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not rename '{}': {}.", assetPath, error.message()));
}

void EditorLayer::DrawDeleteAssetPopup()
{
	if (mAssetToDelete.empty())
		return;

	// Opened here, at the root, and not from the row's context menu: that menu closes on click and takes
	// its ID scope — and the modal — with it.
	if (!ImGui::IsPopupOpen("Delete Asset"))
		ImGui::OpenPopup("Delete Asset");

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Delete Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	const std::filesystem::path full = ProjectPanelDirectory() / mAssetToDelete;

	std::error_code error;
	const bool folder = std::filesystem::is_directory(full, error);

	ImGui::TextUnformatted(mAssetToDelete.c_str());

	// A file goes to the recycle bin nowhere: this is the project's disk, and the editor's undo history
	// is about the scene, not about the files under it. Say so before doing it.
	ImGui::TextDisabled(folder
		? "The folder and everything in it will be deleted. This cannot be undone."
		: "The file will be deleted. This cannot be undone.");

	ImGui::Spacing();

	if (ImGui::Button("Delete", ImVec2(96.0f, 0.0f)))
	{
		std::filesystem::remove_all(full, error);
		mProjectDirty = true;

		if (error)
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not delete '{}': {}.", mAssetToDelete, error.message()));
		else
			Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[Editor] Deleted '{}'.", mAssetToDelete));

		mAssetToDelete.clear();
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)))
	{
		mAssetToDelete.clear();
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

void EditorLayer::DrawShortcuts()
{
	if (!mShowShortcuts)
		return;

	ImGui::SetNextWindowSize(ImVec2(800.0f, 600.0f), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(kShortcutsWindow, &mShowShortcuts))
	{
		// The rebindable actions, grouped by category (order defines the display order).
		struct Row { ShortcutAction action; const char8* category; const char8* name; };
		static const Row rows[] = {
			{ ShortcutAction::NewProject,      "Project",   "New project..." },
			{ ShortcutAction::OpenProject,     "Project",   "Open a project..." },
			{ ShortcutAction::NewScene,        "Scene",     "New scene" },
				{ ShortcutAction::OpenScene,       "Scene",     "Open a scene" },
				{ ShortcutAction::SaveScene,       "Scene",     "Save the scene" },
				{ ShortcutAction::SaveSceneAs,     "Scene",     "Save the scene as..." },
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
			{ ShortcutAction::FocusSelection,  "Viewport",  "Frame the selection" },
			{ ShortcutAction::Deselect,        "Hierarchy", "Clear the selection" },
				{ ShortcutAction::RenameEntity,    "Hierarchy", "Rename selected entity" },
			{ ShortcutAction::DeleteEntity,    "Hierarchy", "Delete selected entity" },
			{ ShortcutAction::CopyEntity,      "Hierarchy", "Copy selected entity" },
			{ ShortcutAction::PasteEntity,     "Hierarchy", "Paste entity from clipboard" },
			{ ShortcutAction::DuplicateEntity, "Hierarchy", "Duplicate selected entity" },
			{ ShortcutAction::StepFrame,       "Play Mode", "Step one frame" },
			{ ShortcutAction::ToggleColliders, "Viewport",  "Toggle collider hitboxes" },
			{ ShortcutAction::CompileModule,   "Game",      "Compile the game module" },
			{ ShortcutAction::ReloadModule,    "Game",      "Reload the game module" },
			{ ShortcutAction::ToggleHierarchy,  "Panels",   "Show/hide Scene Hierarchy" },
			{ ShortcutAction::ToggleProperties, "Panels",   "Show/hide Properties" },
			{ ShortcutAction::ToggleProject,    "Panels",   "Show/hide Project" },
			{ ShortcutAction::ToggleConsole,    "Panels",   "Show/hide Console" },
			{ ShortcutAction::ToggleStatistics, "Panels",   "Show/hide Statistics" },
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
				ImGui::PushStyleColor(ImGuiCol_Button, EditorGui::GetAccent());

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
	mUndoStack.push_back({ EditKind::Scene, SceneSerializer::SerializeToString(mScene) });

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

	mUndoStack.push_back({ EditKind::Scene, mPendingSnapshot });

	if (mUndoStack.size() > kMaxUndo)
		mUndoStack.erase(mUndoStack.begin());

	mRedoStack.clear();
}

EditorLayer::EditState EditorLayer::CaptureCurrent(EditKind kind) const
{
	if (kind == EditKind::Layout)
		return { EditKind::Layout, ImGui::SaveIniSettingsToMemory(nullptr) };

	return { EditKind::Scene, SceneSerializer::SerializeToString(mScene) };
}

void EditorLayer::RestoreState(const EditState& state)
{
	if (state.kind == EditKind::Layout)
	{
		ImGui::LoadIniSettingsFromMemory(state.data.c_str(), state.data.size());
		return;
	}

	// The scene is rebuilt from scratch, so any selected-entity pointer becomes stale.
	SetSelection(nullptr);
	SceneSerializer::DeserializeFromString(mScene, state.data);
}

void EditorLayer::Undo()
{
	if (mUndoStack.empty())
		return;

	const EditState state = mUndoStack.back();
	mUndoStack.pop_back();

	// What is undone is put on the redo stack as it stands now, in the same kind, so redo puts it back.
	mRedoStack.push_back(CaptureCurrent(state.kind));
	RestoreState(state);
}

void EditorLayer::Redo()
{
	if (mRedoStack.empty())
		return;

	const EditState state = mRedoStack.back();
	mRedoStack.pop_back();

	mUndoStack.push_back(CaptureCurrent(state.kind));
	RestoreState(state);
}

namespace
{
	// User-customized shortcuts, kept under Data/ alongside the UI layout (see EditorGui::Init).
	std::filesystem::path ShortcutsFile()
	{
		return EditorDataDirectory() / "lion-shortcuts.ini";
	}
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
	set(ShortcutAction::ToggleShortcuts, ImGuiKey_F10);
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

	// Alt+1..5, in the order the panels are listed (see kPanels).
	set(ShortcutAction::ToggleHierarchy, ImGuiKey_1, false, false, true);
	set(ShortcutAction::ToggleProperties, ImGuiKey_2, false, false, true);
	set(ShortcutAction::ToggleProject, ImGuiKey_3, false, false, true);
	set(ShortcutAction::ToggleConsole, ImGuiKey_4, false, false, true);
	set(ShortcutAction::ToggleStatistics, ImGuiKey_5, false, false, true);

	set(ShortcutAction::Deselect, ImGuiKey_Escape);

	// The scene keys every editor has, bound where every editor binds them.
	set(ShortcutAction::NewScene, ImGuiKey_N, true);
	set(ShortcutAction::OpenScene, ImGuiKey_O, true);
	set(ShortcutAction::SaveScene, ImGuiKey_S, true);
	set(ShortcutAction::SaveSceneAs, ImGuiKey_S, true, true);

	// The project keys sit one Shift above the scene keys: the bigger thing, the bigger chord.
	set(ShortcutAction::NewProject, ImGuiKey_N, true, true);
	set(ShortcutAction::OpenProject, ImGuiKey_O, true, true);

	// F frames the selection, where every 3D and 2D editor has put it.
	set(ShortcutAction::FocusSelection, ImGuiKey_F);
}

void EditorLayer::SetSelection(const Reference<Entity>& entity)
{
	mSelection.clear();

	if (entity)
		mSelection.push_back(entity);

	mSelectedEntity = entity;
}

void EditorLayer::AddToSelection(const Reference<Entity>& entity)
{
	if (!entity)
		return;

	const auto it = std::find(mSelection.begin(), mSelection.end(), entity);

	if (it != mSelection.end())
	{
		// Ctrl-clicking something already selected takes it out — including the primary, which then
		// falls back to whatever is left, because a selection with no primary is a selection with no
		// Inspector.
		mSelection.erase(it);
		mSelectedEntity = mSelection.empty() ? nullptr : mSelection.back();
		return;
	}

	mSelection.push_back(entity);
	mSelectedEntity = entity;
}

void EditorLayer::SelectRangeTo(const Reference<Entity>& entity)
{
	if (!entity || !mSelectedEntity)
	{
		SetSelection(entity);
		return;
	}

	// A range is a range of *rows*, so it runs over the entities in the order the scene keeps them —
	// which is the order the Hierarchy lists them. The scene holds them in a list, so the two ends are
	// found by counting rather than by subtracting one iterator from another.
	const auto& entities = mScene->GetEntities();
	int32 anchor = -1;
	int32 target = -1;
	int32 index = 0;

	for (const auto& candidate : entities)
	{
		if (candidate == mSelectedEntity) anchor = index;
		if (candidate == entity)          target = index;

		index++;
	}

	if (anchor < 0 || target < 0)
	{
		SetSelection(entity);
		return;
	}

	const int32 first = std::min(anchor, target);
	const int32 last = std::max(anchor, target);

	mSelection.clear();
	index = 0;

	for (const auto& candidate : entities)
	{
		if (index >= first && index <= last)
			mSelection.push_back(candidate);

		index++;
	}

	mSelectedEntity = entity;
}

void EditorLayer::RenameSelection(const std::string& name, const Reference<Entity>& renamed)
{
	// Renaming one thing renames one thing. Renaming several gives them all the name that was typed, and
	// Windows' answer to several things wanting one name is a number after it.
	if (mSelection.size() <= 1 || !IsSelected(renamed.get()))
	{
		renamed->SetName(name);
		return;
	}

	for (const auto& entity : mSelection)
		entity->SetName(NumberedName(mScene, name));
}

bool EditorLayer::IsSelected(const Entity* entity) const
{
	for (const auto& selected : mSelection)
		if (selected.get() == entity)
			return true;

	return false;
}

Reference<Entity> EditorLayer::CreateEntity(Entity* parent, const Vector* position)
{
	RecordSnapshot();

	auto entity = MakeReference<Entity>();
	mScene->Add(entity);

	if (parent)
		entity->SetParent(parent);

	if (position)
		entity->SetWorldPosition(Vector2(*position));

	SetSelection(entity);
	mRenamingEntity = entity;   // Let the user name it right away.
	mRenameFocus = true;

	return entity;
}

void EditorLayer::CreateFolder()
{
	RecordSnapshot();

	auto folder = MakeReference<Entity>();
	folder->SetFolder(true);
	folder->SetName("Folder");
	mScene->Add(folder);

	SetSelection(folder);
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
		pasted->SetName(NumberedName(mScene, pasted->GetName()));
		SetSelection(pasted);
	}
}

void EditorLayer::DuplicateEntity()
{
	if (mSelection.empty())
		return;

	RecordSnapshot();

	// The whole selection is duplicated, and the copies become the selection — duplicating five things
	// and being left holding the originals is not what anyone meant by it.
	const std::vector<Reference<Entity>> originals = mSelection;
	mSelection.clear();

	for (const auto& original : originals)
	{
		const std::string data = SceneSerializer::SerializeEntityToString(original);

		if (Reference<Entity> copy = SceneSerializer::DeserializeEntityFromString(mScene, data))
		{
			copy->SetName(NumberedName(mScene, original->GetName()));
			mSelection.push_back(copy);
		}
	}

	mSelectedEntity = mSelection.empty() ? nullptr : mSelection.back();
}

void EditorLayer::InitShortcuts()
{
	ResetShortcutsToDefault();
	LoadShortcuts();
}

void EditorLayer::LoadShortcuts()
{
	std::ifstream file(ShortcutsFile());

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
	std::filesystem::create_directories(EditorDataDirectory(), error);

	std::ofstream file(ShortcutsFile());

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

std::string EditorLayer::ShortcutText(ShortcutAction action) const
{
	const Keybind& bind = mBinds[static_cast<int>(action)];

	// A menu says nothing where there is nothing to say: an action nobody has bound has no key to show.
	return bind.key == ImGuiKey_None ? std::string() : KeybindToString(bind);
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

	// Panel visibility, from the same table the View menu is built from.
	for (const Panel& panel : kPanels)
		if (IsShortcutPressed(panel.shortcut))
			this->*panel.visible = !(this->*panel.visible);

	// Escape drops the selection — but only when it has nothing else to do. It is also what closes a popup
	// and what abandons a rename, and a key that does two things at once does neither of them predictably:
	// the context menu would vanish and take the selection with it. A rename is a text field, so it has
	// already been ruled out above.
	if (IsShortcutPressed(ShortcutAction::Deselect) && !mSelection.empty() &&
		!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
		SetSelection(nullptr);

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

	if (IsShortcutPressed(ShortcutAction::NewScene)) NewScene();
	if (IsShortcutPressed(ShortcutAction::OpenScene)) OpenScene();
	if (IsShortcutPressed(ShortcutAction::SaveScene)) SaveScene();
	if (IsShortcutPressed(ShortcutAction::SaveSceneAs)) SaveSceneAs();
	if (IsShortcutPressed(ShortcutAction::FocusSelection)) FocusViewportOnSelection();
	if (IsShortcutPressed(ShortcutAction::NewProject)) mOpenProjectManagerPopup = true;
	if (IsShortcutPressed(ShortcutAction::OpenProject)) BrowseForProject();

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

		// Del deletes what is selected, all of it — the same rule the context menu follows.
		for (const auto& entity : mSelection)
			mScene->Remove(entity);

		mScene->FlushRemovals();
		SetSelection(nullptr);
	}
}

void EditorLayer::DrawViewport()
{
	// The viewport image fills the window edge to edge. Begin() captures the padding, so it is
	// restored right away: popups opened from here (e.g. Settings) then get the normal padding.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(kViewportWindow);
	ImGui::PopStyleVar();

	const ImVec2 available = ImGui::GetContentRegionAvail();
	mViewportSize = { available.x, available.y };

	// Display the framebuffer's color texture, flipped vertically (OpenGL is bottom-up).
	const auto textureId = static_cast<ImTextureID>(mFramebuffer->GetColorAttachment());
	ImGui::Image(textureId, available, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

	const ImVec2 imageMin = ImGui::GetItemRectMin();
	const ImVec2 imageSize = ImGui::GetItemRectSize();
	const bool imageHovered = ImGui::IsItemHovered();

	// Kept for the dim: it darkens everything the game is not.
	mViewportImageMin = imageMin;
	mViewportImageMax = ImVec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y);

	HandleViewportNavigation(imageMin, imageSize, imageHovered);

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

			// The gizmo sits on one entity, and the drag belongs to the whole selection: what the primary
			// changed by is what every one of them changes by. A delta, and not the primary's new value —
			// they are in different places, and dragging five entities must not stack them on one.
			const Vector moved(newTranslation[0] - position.x, newTranslation[1] - position.y, 0.0f);
			const float32 turned = newRotation[2] - rotationZ;

			// Scale is a ratio for the same reason: an entity twice the size of another stays twice the size.
			const float32 scaleX = (std::fabs(scale.x) > 0.0001f) ? newScale[0] / scale.x : 1.0f;
			const float32 scaleY = (std::fabs(scale.y) > 0.0001f) ? newScale[1] / scale.y : 1.0f;

			for (const auto& entity : mSelection)
			{
				if (!entity || entity->IsFolder())
					continue;

				const Vector2 entityPosition = entity->GetWorldPosition();
				const Vector2 entityScale = entity->GetWorldScale();

				switch (mTool)
				{
					case Tool::Move:
						entity->SetWorldPosition(Vector2(entityPosition.x + moved.x, entityPosition.y + moved.y));
						break;

					case Tool::Rotate:
						entity->SetWorldRotation(entity->GetWorldRotation() + turned);
						break;

					case Tool::Scale:
						entity->SetWorldScale(Vector2(entityScale.x * scaleX, entityScale.y * scaleY));
						break;

					default:
						break;
				}
			}
		}
	}

	// What is selected, outlined where it is — before the colliders, so a collider's line wins where the
	// two land on the same pixel: the collider is the debug view, and a debug view nobody can read is off.
	DrawSelectionOutline(imageMin, imageSize);

	// A camera is a thing with a reach, and the reach is the part you cannot see without being shown it.
	// Not while the game runs: then you are already looking through it.
	if (!mPlaying)
		DrawCameraOverlays(imageMin, imageSize);

	if (mShowColliders)
		DrawColliderOverlays(imageMin, imageSize);

	// Play mode is signalled by an outline around the viewport, in the engine's colour. It is inset by half
	// its thickness so the whole stroke stays inside the image instead of being clipped in half.
	if (mPlaying)
	{
		constexpr float32 thickness = 4.0f;
		const float32 inset = thickness * 0.5f;

		ImGui::GetWindowDrawList()->AddRect(
			ImVec2(imageMin.x + inset, imageMin.y + inset),
			ImVec2(imageMin.x + imageSize.x - inset, imageMin.y + imageSize.y - inset),
			ImGui::ColorConvertFloat4ToU32(EditorGui::GetAccent()), 0.0f, 0, thickness);
	}

	DrawViewportToolbar(imageMin, imageSize);

	// The Hierarchy's menu, opened over the scene itself. Where the mouse was when it opened is captured
	// then and there — the menu is a window, and the cursor walks away from the spot the moment it opens.
	//
	// No gizmo guard on the capture: the gizmo is where the *selected* entity is, and a right-click there is
	// still a request to create the next one under the cursor. Guarding it left the position at wherever the
	// last menu opened, so a new entity landed away from the mouse — the "not exactly at my cursor".
	if (imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		const ImVec2 mouse = ImGui::GetMousePos();
		const glm::mat4 inverseViewProjection = glm::inverse(mCamera->GetProjectionMatrix() * mCamera->GetViewMatrix());

		const float32 ndcX = ((mouse.x - imageMin.x) / imageSize.x) * 2.0f - 1.0f;
		const float32 ndcY = 1.0f - ((mouse.y - imageMin.y) / imageSize.y) * 2.0f;  // Screen space is top-down.
		const glm::vec4 world = inverseViewProjection * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

		mViewportMenuPosition = Vector(world.x / world.w, world.y / world.w, 0.0f);
	}

	// Over the window rather than over the image: the image is an item, and a menu that refuses to open
	// over items would never open over the one thing this menu is about.
	if (ImGui::BeginPopupContextWindow("ViewportContext", ImGuiPopupFlags_MouseButtonRight))
	{
		DrawEntityMenuItems(nullptr, &mViewportMenuPosition, true);
		ImGui::EndPopup();
	}

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

			Reference<Entity> picked;

			for (const auto& entity : mScene->GetEntities())
			{
				if (entity->GetId() == id)
				{
					picked = entity;
					break;
				}
			}

			// The viewport picks the same way the Hierarchy clicks: Ctrl adds to the selection.
			if (picked && ImGui::GetIO().KeyCtrl)
				AddToSelection(picked);
			else
				SetSelection(picked);
		}
	}

	ImGui::End();
}

Lion::float32 EditorLayer::ToolbarDockWidth(int count)
{
	return count * kDockButton + (count - 1) * kDockSpacing + kDockPadding * 2.0f;
}

void EditorLayer::BeginToolbarDock(const char8* id, const ImVec2& position, int count)
{
	const ImVec2 size(ToolbarDockWidth(count), kDockButton + kDockPadding * 2.0f);

	// The panel is painted, not begun as a child window: a child would take the mouse wheel and the clicks
	// that belong to the scene under it, and it would need a scroll region it will never scroll.
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y),
		IM_COL32(26, 27, 31, 235), 6.0f);

	drawList->AddRect(position, ImVec2(position.x + size.x, position.y + size.y),
		IM_COL32(0, 0, 0, 110), 6.0f);

	ImGui::PushID(id);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kDockSpacing, kDockSpacing));

	ImGui::SetCursorScreenPos(ImVec2(position.x + kDockPadding, position.y + kDockPadding));
}

void EditorLayer::EndToolbarDock()
{
	ImGui::PopStyleVar();
	ImGui::PopID();
}

// One dock button: a 24px icon centred in a square, with its own hover and active background. Drawn rather
// than an ImGui::Button(icon), so the icon is a true 24px and sits in the middle of the square instead of
// riding low on a text baseline.
bool EditorLayer::ToolbarButton(const char8* id, const char8* icon, bool active, bool enabled, const char8* tooltip)
{
	ImGui::BeginDisabled(!enabled);

	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const bool clicked = ImGui::InvisibleButton(id, ImVec2(kDockButton, kDockButton));
	const bool hovered = ImGui::IsItemHovered();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 max(origin.x + kDockButton, origin.y + kDockButton);

	if (active)
		drawList->AddRectFilled(origin, max, ImGui::ColorConvertFloat4ToU32(EditorGui::GetAccent()), 4.0f);
	else if (hovered && enabled)
		drawList->AddRectFilled(origin, max, IM_COL32(255, 255, 255, 24), 4.0f);

	const ImU32 color = ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
	DrawIcon(origin, kDockButton, icon, color, kIconSize);

	if (hovered && enabled && tooltip)
		ImGui::SetTooltip("%s", tooltip);

	ImGui::EndDisabled();
	return clicked;
}

void EditorLayer::DrawViewportToolbar(const ImVec2& imageMin, const ImVec2& imageSize)
{
	// Two docks and a button, floating over the image: what you do to the scene on the left, what you do to
	// time along the top, and how the scene is shown on the right. They are docks and not loose buttons
	// because a row of buttons on top of a picture reads as part of the picture.

	// Tools, top-left.
	struct ToolButton { Tool tool; const char8* icon; const char8* tooltip; };
	static const ToolButton tools[] = {
		{ Tool::Select, ICON_MDI_CURSOR_DEFAULT,   "Select entities (Q)"    },
		{ Tool::Move,   ICON_MDI_ARROW_ALL,        "Move the selection (W)" },
		{ Tool::Rotate, ICON_MDI_ROTATE_RIGHT,     "Rotate the selection (E)" },
		{ Tool::Scale,  ICON_MDI_ARROW_EXPAND_ALL, "Scale the selection (R)"  },
	};

	BeginToolbarDock("##tools", ImVec2(imageMin.x + kDockMargin, imageMin.y + kDockMargin), IM_ARRAYSIZE(tools));

	for (const ToolButton& button : tools)
	{
		if (button.tool != tools[0].tool)
			ImGui::SameLine();

		if (ToolbarButton(button.icon, button.icon, mTool == button.tool, true, button.tooltip))
			mTool = button.tool;
	}

	EndToolbarDock();

	// Play, step, pause, stop — centred along the top edge, where the thing they act on is.
	const float32 playDockWidth = ToolbarDockWidth(4);

	BeginToolbarDock("##play",
		ImVec2(imageMin.x + (imageSize.x - playDockWidth) * 0.5f, imageMin.y + kDockMargin), 4);

	// Play doubles as "resume" while paused, so it stays enabled in that state.
	if (ToolbarButton("##play", ICON_MDI_PLAY, false, !(mPlaying && !mPaused), "Run the scene simulation (F5)"))
		StartPlay();

	// Step advances a halted simulation one frame at a time, so it only means anything while playing.
	ImGui::SameLine();
	if (ToolbarButton("##step", ICON_MDI_STEP_FORWARD, false, mPlaying, "Advance the simulation by one frame (F6)"))
		StepOneFrame();

	ImGui::SameLine();
	if (ToolbarButton("##pause", ICON_MDI_PAUSE, mPaused, mPlaying, mPaused ? "Resume the simulation (F7)" : "Pause the simulation (F7)"))
		TogglePause();

	ImGui::SameLine();
	if (ToolbarButton("##stop", ICON_MDI_STOP, false, mPlaying, "Stop and return to the edited state (F8)"))
		StopPlay();

	EndToolbarDock();

	// How the scene is shown, top-right. One button, in a dock of its own, so it sits on the same line as
	// the other two rather than floating beside them.
	BeginToolbarDock("##settings",
		ImVec2(imageMin.x + imageSize.x - ToolbarDockWidth(1) - kDockMargin, imageMin.y + kDockMargin), 1);

	if (ToolbarButton("##cog", ICON_MDI_COG, false, true, "Viewport display options"))
		ImGui::OpenPopup("ViewportSettings");

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

	EndToolbarDock();
}

void EditorLayer::DrawSelectionOutline(const ImVec2& imageMin, const ImVec2& imageSize)
{
	if (mSelection.empty() || imageSize.x <= 0.0f || imageSize.y <= 0.0f)
		return;

	// Every selected entity gets an outline, not just the one the Inspector is showing: a selection you
	// cannot see the extent of is a selection you are about to move by accident.
	const glm::mat4 viewProjection = mCamera->GetProjectionMatrix() * mCamera->GetViewMatrix();
	const ImVec4 accent = EditorGui::GetAccent();
	const ImU32 color = ImGui::ColorConvertFloat4ToU32(accent);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const auto worldToScreen = [&](float32 worldX, float32 worldY) -> ImVec2
	{
		const glm::vec4 clip = viewProjection * glm::vec4(worldX, worldY, 0.0f, 1.0f);
		return ImVec2(
			imageMin.x + (clip.x / clip.w * 0.5f + 0.5f) * imageSize.x,
			imageMin.y + (1.0f - (clip.y / clip.w * 0.5f + 0.5f)) * imageSize.y);   // Flip Y for screen space.
	};

	for (const auto& entity : mSelection)
	{
		// A folder is not in the scene; it is a place in the Hierarchy, and it has nothing to draw around.
		if (!entity || entity->IsFolder())
			continue;

		// The box is the one the entity actually occupies, which is what it draws: a sprite is its texture's
		// size times the transform's scale, not the scale on its own — every entity in the demo scene has a
		// scale of 1, and an outline drawn from that is a dot. An entity with nothing to draw still gets a
		// box, from its collider or, failing that, a small square: an outline you cannot see is not one.
		const Vector position = entity->GetWorldPosition();
		const Vector scale = entity->GetWorldScale();
		const float32 angle = glm::radians(entity->GetWorldRotation());
		const float32 cosAngle = std::cos(angle);
		const float32 sinAngle = std::sin(angle);

		constexpr float32 kEmptyExtent = 24.0f;
		float32 width = kEmptyExtent;
		float32 height = kEmptyExtent;

		if (const SpriteRenderer* sprite = entity->GetComponent<SpriteRenderer>())
		{
			const Size size = sprite->GetSize();

			if (size.width > 0.0f && size.height > 0.0f)
			{
				width = size.width;
				height = size.height;
			}
		}
		else if (const BoxCollider2D* box = entity->GetComponent<BoxCollider2D>())
		{
			width = box->GetWidth();
			height = box->GetHeight();
		}
		else if (const CircleCollider2D* circle = entity->GetComponent<CircleCollider2D>())
		{
			width = circle->GetRadius() * 2.0f;
			height = width;
		}

		const float32 halfWidth = width * std::fabs(scale.x) * 0.5f;
		const float32 halfHeight = height * std::fabs(scale.y) * 0.5f;

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

		drawList->AddPolyline(points, 4, color, ImDrawFlags_Closed, 2.0f);
	}
}

void EditorLayer::DrawCameraOverlays(const ImVec2& imageMin, const ImVec2& imageSize)
{
	if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
		return;

	const glm::mat4 viewProjection = mCamera->GetProjectionMatrix() * mCamera->GetViewMatrix();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// The editor's overlay palette, one hue per thing it means. Orange is the selection's and green is
	// the colliders', so a camera takes the blue nobody was using — the colour of a lens — and its limit
	// the violet next to it: two neighbours read as one idea, and neither is mistaken for the other two.
	// (Godot's own purple and gold sit too close to this editor's accent to stay legible beside it.)
	constexpr ImU32 kCameraColor = IM_COL32(86, 182, 232, 255);
	constexpr ImU32 kLimitColor = IM_COL32(167, 132, 239, 255);

	const auto worldToScreen = [&](float32 worldX, float32 worldY) -> ImVec2
	{
		const glm::vec4 clip = viewProjection * glm::vec4(worldX, worldY, 0.0f, 1.0f);
		const float32 ndcX = clip.x / clip.w;
		const float32 ndcY = clip.y / clip.w;
		return ImVec2(
			imageMin.x + (ndcX * 0.5f + 0.5f) * imageSize.x,
			imageMin.y + (1.0f - (ndcY * 0.5f + 0.5f)) * imageSize.y);
	};

	for (const auto& entity : mScene->GetEntities())
	{
		const Camera2D* camera = entity->GetComponent<Camera2D>();

		if (!camera)
			continue;

		// The limit first, so the framing rectangle reads on top of it.
		if (camera->HasLimit())
		{
			const Vector minimum(std::min(camera->GetLimitLeft(), camera->GetLimitRight()),
				std::min(camera->GetLimitBottom(), camera->GetLimitTop()));
			const Vector maximum(std::max(camera->GetLimitLeft(), camera->GetLimitRight()),
				std::max(camera->GetLimitBottom(), camera->GetLimitTop()));

			// Dashed, because a limit is a rule rather than a thing: it is the one line in the viewport
			// that marks where something may not go.
			const ImVec2 corners[4] = {
				worldToScreen(minimum.x, maximum.y), worldToScreen(maximum.x, maximum.y),
				worldToScreen(maximum.x, minimum.y), worldToScreen(minimum.x, minimum.y) };

			for (int32 side = 0; side < 4; ++side)
			{
				const ImVec2 from = corners[side];
				const ImVec2 to = corners[(side + 1) % 4];
				const float32 length = ImSqrt(ImLengthSqr(ImVec2(to.x - from.x, to.y - from.y)));
				const int32 dashes = ImMax(static_cast<int32>(length / 12.0f), 1);

				for (int32 dash = 0; dash < dashes; dash += 2)
				{
					const float32 start = static_cast<float32>(dash) / dashes;
					const float32 end = ImMin(static_cast<float32>(dash + 1) / dashes, 1.0f);

					drawList->AddLine(
						ImVec2(from.x + (to.x - from.x) * start, from.y + (to.y - from.y) * start),
						ImVec2(from.x + (to.x - from.x) * end, from.y + (to.y - from.y) * end),
						kLimitColor, 1.5f);
				}
			}
		}

		// What the camera sees: its view rectangle, at its own position and zoom — where it would be
		// looking if the game were running, limits and all.
		const glm::vec2 center = camera->GetTargetPosition();
		const glm::vec2 half = camera->GetViewSize() * 0.5f;

		const ImVec2 topLeft = worldToScreen(center.x - half.x, center.y + half.y);
		const ImVec2 bottomRight = worldToScreen(center.x + half.x, center.y - half.y);

		drawList->AddRect(topLeft, bottomRight, kCameraColor, 0.0f, 0, 2.0f);

		// A cross at the centre says which point the camera is on, which a rectangle alone does not.
		const ImVec2 eye = worldToScreen(center.x, center.y);
		constexpr float32 kArm = 6.0f;

		drawList->AddLine(ImVec2(eye.x - kArm, eye.y), ImVec2(eye.x + kArm, eye.y), kCameraColor, 1.5f);
		drawList->AddLine(ImVec2(eye.x, eye.y - kArm), ImVec2(eye.x, eye.y + kArm), kCameraColor, 1.5f);
	}
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
	if (!mShowHierarchy)
		return;

	ImGui::Begin(kHierarchyWindow, &mShowHierarchy);

	const ImGuiStyle& style = ImGui::GetStyle();

	// Add, then what you are looking for, then what you are looking in. The icon is part of the label: it is
	// a character, so the button lays it out beside the word the way it lays out the word itself.
	if (ImGui::Button(ICON_MDI_PLUS "  Add"))
		CreateEntity();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Create an entity");

	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##search", ICON_MDI_MAGNIFY "  Search...", mHierarchyFilter, IM_ARRAYSIZE(mHierarchyFilter));

	// The scene these entities are in is named in the title bar, where what is open belongs. It was named
	// here as well, which is a panel answering a question about the window.

	// Children are stored as raw pointers; this maps them back to the scene's owning references.
	mEntityLookup.clear();
	for (const auto& entity : mScene->GetEntities())
		mEntityLookup.emplace(entity.get(), entity);

	mEntityToDelete = nullptr;
	mReparentChild = nullptr;
	mReparentTarget = nullptr;
	mReparentRequested = false;

	// The tree scrolls; the count below it does not. It is the one number that is always true about a
	// scene, so it is always on screen.
	//
	// The header is a bar, and a bar that stops short of the edges it is meant to sit against reads as
	// floating above them. So the tree runs the panel's full width, from its left edge to its right: the
	// child drops the panel's own margin (which is why it is placed at x = 0 and given the whole window's
	// width) and its own padding with it.
	//
	// Only the sides, though. The scene's name keeps the same air under it that it has over it — a line of
	// text pinned against the bar below reads as belonging to it, and it belongs to what is above.
	const float32 footerHeight = ImGui::GetFrameHeightWithSpacing();

	ImGui::SetCursorPosX(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("HierarchyTree", ImVec2(ImGui::GetWindowWidth(), -footerHeight), ImGuiChildFlags_None);
	ImGui::PopStyleVar();

	const int32 count = static_cast<int32>(mScene->GetEntities().size());

	// Two columns: what a thing is called, and whether it is drawn. The eye is a property of the entity
	// and not of the editor, so the game can reach for it too.
	//
	// No banding and no rules: a striped list is a table pretending it has more to say than one column of
	// names, and the lines around it only fence off what was already fenced by the panel it is in.
	//
	// And no room between the rows. A cell's vertical padding is dead space that belongs to neither row,
	// so the cursor leaves one entity and hovers nothing before it reaches the next. A list of names is
	// read by running down it — the rows have to touch, the way the console's lines do.
	// NoPadOuterX: a table keeps a margin at its outer edges, and that margin is what leaves the header
	// bar hanging inside the panel instead of sitting against it.
	//
	// The rows take their height from the node inside them, so this is where that height is set: a couple
	// of pixels above and below the text, which is enough to stop the list reading as a wall of names and
	// far short of a field's padding, which a row is not.
	constexpr float32 kRowPadding = 2.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, kRowPadding));

	if (ImGui::BeginTable("Entities", 2, ImGuiTableFlags_NoPadOuterX))
	{
		// Wide enough for its own header: a column called Visibility that reads "Visi..." is a column that
		// gave its name away to save a dozen pixels.
		const float32 visibilityWidth = ImMax(ImGui::CalcTextSize("Visibility").x, RowEndSlot()) + style.CellPadding.x * 2.0f;

		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Visibility", ImGuiTableColumnFlags_WidthFixed, visibilityWidth);

		// The header row is drawn by hand for two reasons. A tree node starts its label past the arrow, so
		// a header written at the column's edge sits to the left of every name under it. And the rows carry
		// no cell padding — they get their height from the node inside them — so the header has to ask for
		// its own, which it does by naming the height it wants and sitting in the middle of it.
		const float32 headerHeight = ImGui::GetFrameHeightWithSpacing();
		const float32 headerText = ImFloor((headerHeight - ImGui::GetFontSize()) * 0.5f);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers, headerHeight);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPos(ImVec2(
			ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing(),
			ImGui::GetCursorPosY() + headerText));
		ImGui::TableHeader("Name");

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + headerText);
		ImGui::TableHeader("Visibility");

		for (const auto& entity : mScene->GetEntities())
			if (entity->GetParent() == nullptr)
				DrawEntityNode(entity);

		ImGui::EndTable();
	}

	ImGui::PopStyleVar(2);

	if (count == 0)
		ImGui::TextDisabled("Empty scene — right-click to create an entity.");

	// Right-click empty space in the panel to create an entity (Unity/Hazel-style).
	if (ImGui::BeginPopupContextWindow("HierarchyContext",
		ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		DrawEntityMenuItems(nullptr, nullptr);
		ImGui::EndPopup();
	}

	// Click empty space inside the panel to deselect.
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		SetSelection(nullptr);

	ImGui::EndChild();

	ImGui::Separator();

	if (mSelection.empty())
		ImGui::TextDisabled("%d %s", count, (count == 1) ? "Entity" : "Entities");
	else
		ImGui::TextDisabled("%d %s | %d Selected", count, (count == 1) ? "Entity" : "Entities",
			static_cast<int32>(mSelection.size()));

	// Deferred hierarchy edits (never mutate the tree while iterating it above).
	if (mReparentRequested && mReparentChild)
	{
		RecordSnapshot();

		// A drag carries the whole selection when the thing dragged is part of it, which is the only
		// reading of "drag them onto a folder" that does not mean doing it one at a time.
		const Reference<Entity> dragged = mEntityLookup.count(mReparentChild) ? mEntityLookup[mReparentChild] : nullptr;
		const bool dragSelection = dragged && IsSelected(mReparentChild);

		if (dragSelection)
		{
			for (const auto& entity : mSelection)
				if (entity.get() != mReparentTarget && !(mReparentTarget && mReparentTarget->IsDescendantOf(entity.get())))
					entity->SetParent(mReparentTarget);
		}
		else
		{
			mReparentChild->SetParent(mReparentTarget);
		}
	}

	if (mReorderRequested && mReorderMoved && mReorderBefore && mReorderMoved != mReorderBefore)
	{
		// Dropping a row onto its own descendant would ask the hierarchy to contain itself.
		if (!mReorderBefore->IsDescendantOf(mReorderMoved))
		{
			RecordSnapshot();

			const Reference<Entity> moved = mEntityLookup.count(mReorderMoved) ? mEntityLookup[mReorderMoved] : nullptr;

			if (moved)
			{
				// Landing between two rows means joining their parent first: an entity cannot sit in an
				// order it is not part of.
				if (moved->GetParent() != mReorderParent)
					moved->SetParent(mReorderParent);

				// Dropped below a row, it goes before whatever follows that row — and past the last row,
				// before nothing, which the scene reads as the end.
				const Entity* before = mReorderBefore;

				if (mReorderAfter)
				{
					before = nullptr;
					bool found = false;

					for (const auto& entity : mScene->GetEntities())
					{
						if (found && entity.get() != moved.get() && entity->GetParent() == mReorderParent)
						{
							before = entity.get();
							break;
						}

						if (entity.get() == mReorderBefore)
							found = true;
					}
				}

				mScene->Reorder(moved, before);
			}
		}
	}

	mReorderRequested = false;
	mReorderMoved = nullptr;
	mReorderBefore = nullptr;
	mReorderParent = nullptr;

	if (mEntityToDelete)
	{
		RecordSnapshot();

		// Deleting one of several deletes all of them, for the same reason dragging one drags all of them.
		const std::vector<Reference<Entity>> doomed = IsSelected(mEntityToDelete.get())
			? mSelection
			: std::vector<Reference<Entity>>{ mEntityToDelete };

		for (const auto& entity : doomed)
		{
			if (mRenamingEntity == entity)
				mRenamingEntity = nullptr;

			mScene->Remove(entity);
		}

		SetSelection(nullptr);
		mScene->FlushRemovals();
		mEntityToDelete = nullptr;
	}

	ImGui::End();
}

void EditorLayer::DrawEntityMenuItems(const Reference<Entity>& target, const Vector* position, bool inViewport)
{
	// One menu, opened from two places. The Hierarchy's version has no position, so an entity lands at
	// the origin; the viewport's has one, so it lands under the mouse — which is the only difference
	// between them, and no reason for a second menu.
	if (target)
	{
		if (ImGui::MenuItem("Rename", "F2"))
		{
			mRenamingEntity = target;
			mRenameFocus = true;
		}

		ImGui::Separator();
	}

	if (ImGui::MenuItem("Create Entity"))
		CreateEntity(nullptr, position);

	if (target && ImGui::MenuItem("Create Entity as Child"))
		CreateEntity(target.get(), nullptr);

	// A folder is a place in the Hierarchy, not a thing in the scene, so the viewport does not offer one.
	if (!inViewport && ImGui::MenuItem("Create Folder"))
		CreateFolder();

	// Copy, paste and duplicate act on the Hierarchy's selection; over the scene they have no subject the
	// mouse is pointing at, so the viewport leaves them to the Hierarchy (and to their shortcuts).
	if (!inViewport)
	{
		ImGui::Separator();

		if (target)
		{
			if (ImGui::MenuItem("Copy", "Ctrl+C"))      CopyEntity();
			if (ImGui::MenuItem("Duplicate", "Ctrl+D")) DuplicateEntity();
		}

		if (ImGui::MenuItem("Paste", "Ctrl+V", false, !mEntityClipboard.empty()))
			PasteEntity();
	}

	if (!target)
		return;

	ImGui::Separator();

	if (ImGui::MenuItem("Unparent", nullptr, false, target->GetParent() != nullptr))
	{
		mReparentChild = target.get();
		mReparentTarget = nullptr;
		mReparentRequested = true;
	}

	if (ImGui::MenuItem("Delete", "Del"))
		mEntityToDelete = target;
}

void EditorLayer::DrawEntityNode(const Reference<Entity>& entity)
{
	// The search hides what does not match — but never a node whose child does, or a filter would tell
	// you the thing you are looking for is not there when it is only nested.
	if (!MatchesHierarchyFilter(entity.get(), mHierarchyFilter))
		return;

	ImGui::PushID(entity->GetId());
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);

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
			RenameSelection(buffer, entity);
			mRenamingEntity = nullptr;
		}

		ImGui::PopID();
		return;
	}

	// The row breathes through the node, not around it: FramePadding makes the node itself taller, so the
	// room it takes is room it can be hovered in. Padding the cell instead would put that room between the
	// rows, where it belongs to neither of them and the cursor hovers nothing crossing it.
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAllColumns |
		ImGuiTreeNodeFlags_FramePadding;

	if (entity->GetChildren().empty())
		flags |= ImGuiTreeNodeFlags_Leaf;

	if (IsSelected(entity.get()))
		flags |= ImGuiTreeNodeFlags_Selected;

	// A search that hid the branch would hide the match inside it, so a filtered tree opens itself.
	if (mHierarchyFilter[0] != '\0')
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);

	const std::string& name = entity->GetName();
	const bool folder = entity->IsFolder();

	// What the row is, said by an icon rather than by a colour: a folder that is open looks open, and a
	// scene object looks like an object. The yellow that used to say it was a thing you had to learn.
	//
	// A hidden or disabled entity is dimmed — the eye says why, and the name says so at a glance.
	const bool dimmed = !folder && (!entity->IsVisible() || !entity->IsEnabled());

	if (dimmed)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

	// A selected row wears the engine's orange — the same colour as its outline in the viewport and the
	// frame around a running game. The selection is one thing; it looks like one thing wherever it shows.
	const ImVec4 accent = EditorGui::GetAccent();
	ImGui::PushStyleColor(ImGuiCol_Header, accent);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IsSelected(entity.get()) ? accent : ImVec4(accent.x, accent.y, accent.z, 0.30f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, accent);

	const bool expanded = folder && ImGui::TreeNodeGetOpen(ImGui::GetID("##node"));
	const char8* icon = folder ? (expanded ? ICON_MDI_FOLDER_OPEN : ICON_MDI_FOLDER) : ICON_MDI_CUBE_OUTLINE;

	// The row icon is drawn by hand, so the label reserves room for it with spaces and the glyph is painted
	// into that gap after the node is laid out — inline in the label it could only be the small merged size.
	const float32 spaceWidth = ImMax(ImGui::CalcTextSize(" ").x, 1.0f);
	const int32 spaces = static_cast<int32>(ImCeil((kIconSize + 6.0f) / spaceWidth));
	const std::string label = std::string(spaces, ' ') + (name.empty() ? "(unnamed)" : name);

	const ImVec2 rowStart = ImGui::GetCursorScreenPos();

	ImGui::SetNextItemAllowOverlap();   // The eye sits in the row the node spans, and gets its own clicks.
	const bool open = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

	// Paint the icon into the space reserved for it: after the expand arrow, centred down the row.
	const float32 iconX = rowStart.x + ImGui::GetTreeNodeToLabelSpacing();
	const ImU32 iconColor = ImGui::GetColorU32(dimmed ? ImGuiCol_TextDisabled : ImGuiCol_Text);
	DrawIcon(ImVec2(iconX, ImGui::GetItemRectMin().y), ImVec2(kIconSize, ImGui::GetItemRectSize().y), icon, iconColor, kIconSize);

	ImGui::PopStyleColor(3);

	if (dimmed)
		ImGui::PopStyleColor();

	// Clicking the label (not the expand arrow) selects. Ctrl adds one, Shift takes everything between.
	//
	// A press on something already selected changes nothing, because a press is where a drag begins: the
	// selection has to still be there when it does, or dragging five things would drag the one grabbed.
	// The press is only answered on release, once it is clear it was a click and not a drag.
	const ImGuiIO& io = ImGui::GetIO();
	const bool toggledOpen = ImGui::IsItemToggledOpen();

	if (ImGui::IsItemClicked() && !toggledOpen)
	{
		if (io.KeyShift)
			SelectRangeTo(entity);
		else if (io.KeyCtrl)
			AddToSelection(entity);
		else if (!IsSelected(entity.get()))
			SetSelection(entity);
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !toggledOpen &&
		!io.KeyShift && !io.KeyCtrl && !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left) &&
		mSelection.size() > 1 && IsSelected(entity.get()))
	{
		SetSelection(entity);
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		mRenamingEntity = entity;
		mRenameFocus = true;
	}

	// Drag a node onto another to reparent it (the child keeps its world transform).
	if (ImGui::BeginDragDropSource())
	{
		Entity* dragged = entity.get();
		ImGui::SetDragDropPayload("LN_ENTITY", &dragged, sizeof(Entity*));

		if (IsSelected(dragged) && mSelection.size() > 1)
			ImGui::Text("Move %d entities", static_cast<int32>(mSelection.size()));
		else
			ImGui::Text("Move %s", name.c_str());

		ImGui::EndDragDropSource();
	}

	// A row is three drop zones down its height, the way Unity's hierarchy is: the top and bottom
	// quarters put the dragged entity *between* rows, and the half in the middle drops it *onto* this one
	// to reparent. One row, both gestures, told apart by where in it the pointer is.
	if (ImGui::BeginDragDropTarget())
	{
		const ImVec2 rowMin = ImGui::GetItemRectMin();
		const ImVec2 rowMax = ImGui::GetItemRectMax();
		const float32 height = ImMax(rowMax.y - rowMin.y, 1.0f);
		const float32 offset = ImClamp((ImGui::GetMousePos().y - rowMin.y) / height, 0.0f, 1.0f);

		constexpr float32 kEdge = 0.25f;
		const bool above = offset < kEdge;
		const bool below = offset > 1.0f - kEdge;

		// The line shows where it would land, drawn over the row rather than in it: the answer to "where
		// does letting go put this?" should not need to be guessed.
		if (above || below)
		{
			const float32 y = above ? rowMin.y : rowMax.y;
			ImGui::GetWindowDrawList()->AddLine(ImVec2(rowMin.x, y), ImVec2(rowMax.x, y),
				ImGui::GetColorU32(EditorGui::GetAccent()), 2.0f);
		}

		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LN_ENTITY"))
		{
			Entity* const dropped = *static_cast<Entity* const*>(payload->Data);

			if (above || below)
			{
				// Ordering is a sibling affair: dropping between two rows means taking this row's parent
				// and sitting next to it, which is what makes a drag reorder rather than reparent.
				mReorderMoved = dropped;
				mReorderBefore = entity.get();
				mReorderAfter = below;
				mReorderParent = entity->GetParent();
				mReorderRequested = true;
			}
			else
			{
				mReparentChild = dropped;
				mReparentTarget = entity.get();
				mReparentRequested = true;
			}
		}

		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem())
	{
		// Right-clicking outside the selection moves it here; right-clicking inside it keeps it, so a
		// menu opened on five selected things still acts on five things.
		if (!IsSelected(entity.get()))
			SetSelection(entity);

		DrawEntityMenuItems(entity, nullptr);
		ImGui::EndPopup();
	}

	// The Visibility column. A folder has nothing to draw, so it has no eye.
	ImGui::TableSetColumnIndex(1);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImMax((ImGui::GetContentRegionAvail().x - RowEndSlot()) * 0.5f, 0.0f));
	AlignRowEndGlyph();   // The row is a frame tall now; the eye is a glyph, and sits in the middle of it.

	if (!folder && EyeButton("##visible", entity->IsVisible()))
	{
		RecordSnapshot();
		const bool visible = !entity->IsVisible();

		// The eye follows the selection when the entity is part of it: hiding five selected things by
		// clicking one of their eyes is the same rule as dragging or deleting them.
		if (IsSelected(entity.get()))
			for (const auto& selected : mSelection)
				selected->SetVisible(visible);
		else
			entity->SetVisible(visible);
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

bool EditorLayer::DrawComponentHeader(const char8* icon, const char8* name, int index, bool& removeRequested, int& dragFrom, int& dragTo)
{
	ImGui::PushID(name);

	const ImGuiStyle& style = ImGui::GetStyle();
	const float32 lineHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

	// The header draws the arrow and the background; the icon and the name are painted onto it, so the icon
	// sits a hair after the arrow instead of at the wide default label offset — that offset was the gap.
	ImGui::SetNextItemAllowOverlap();
	const bool open = ImGui::CollapsingHeader("###header",
		ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap);

	// Exact header bounds, so the remove button sits flush against its right edge.
	const ImVec2 headerMin = ImGui::GetItemRectMin();
	const ImVec2 headerMax = ImGui::GetItemRectMax();

	// Icon then name, laid out by hand: the arrow takes about a font's width, the icon follows it with a
	// little air, and the name follows the icon.
	const float32 iconX = headerMin.x + ImGui::GetFontSize() + 12.0f;
	const float32 rowHeight = headerMax.y - headerMin.y;
	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

	DrawIcon(ImVec2(iconX, headerMin.y), ImVec2(kIconSize, rowHeight), icon, textColor, kIconSize);

	ImGui::GetWindowDrawList()->AddText(
		ImVec2(iconX + kIconSize + 8.0f, ImFloor(headerMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f)),
		textColor, name);

	// A negative index is a fixed component — the Transform — which reorders into nothing and cannot be
	// removed. Everything below is for the ones that can.
	if (index >= 0)
	{
		// Drag the header onto another component's header to reorder.
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("LN_COMPONENT", &index, sizeof(int));
			ImGui::Text("Move %s", name);
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

		// Square remove button sitting on the header row, flush with its right edge.
		ImGui::SameLine();
		ImGui::SetCursorScreenPos(ImVec2(headerMax.x - lineHeight, headerMin.y));
		if (ImGui::Button("X", ImVec2(lineHeight, lineHeight)))
			removeRequested = true;

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Remove component");
	}

	ImGui::PopID();
	return open;
}

bool EditorLayer::DrawFloatProperty(const char8* label, float32& value, float32 speed, float32 minimum,
	float32 maximum, std::optional<float32> defaultValue)
{
	ImGui::PushID(label);

	PropertyLabel(label);
	bool changed = ImGui::DragFloat("##value", &value, speed, minimum, maximum);

	if (ImGui::IsItemActivated()) BeginEdit();

	// Typed arithmetic lands after the widget has read the text: it saw "2", this sees "2+3".
	if (ApplyTypedExpression(ImGui::GetItemID(), value))
	{
		value = ImClamp(value, minimum, maximum);
		changed = true;
	}

	if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

	// A field with no default still keeps the slot, so every field in a component lines up whether or
	// not it has an arrow to show.
	SameLineRowEnd();
	const bool modified = defaultValue.has_value() && (value != *defaultValue);

	if (ResetToDefaultButton("##reset", modified))
	{
		RecordSnapshot();
		value = *defaultValue;
		ImGui::PopID();
		return true;
	}

	ImGui::PopID();
	return changed;
}

bool EditorLayer::DrawVec3Control(const char* label, float values[3], float speed, float resetValue, bool* uniform)
{
	return DrawVectorControl(label, values, 3, speed, resetValue, uniform);
}

bool EditorLayer::DrawVectorControl(const char* label, float* values, int count, float speed, float resetValue, bool* uniform)
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
	const float32 badge = RowEndSlot();
	const float32 badgeOffset = ImFloor((frameHeight - badge) * 0.5f);
	const ImVec2 buttonSize(badge, badge);

	const float32 gap = 2.0f;  // Between a badge and its field, and between one axis and the next.

	// The label takes its column and the row's end keeps two slots — the padlock and the revert arrow —
	// and what is left is split across the three axes. Position and Rotation have no padlock but still
	// hold its slot, so the three rows of a Transform stay in one column.
	const float32 rowEnd = 2.0f * (RowEndSlot() + kRowEndGap);
	const float32 controlsWidth = ImGui::GetContentRegionAvail().x - kVectorLabelWidth - rowEnd;
	const float32 axisWidth = (controlsWidth - static_cast<float32>(count - 1) * gap) / static_cast<float32>(count);
	const float32 dragWidth = ImMax(axisWidth - badge - gap, 12.0f);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine(kVectorLabelWidth);

	// Every item on this row is placed against this baseline: the badges are offset down from it, and
	// the fields sit on it, so a SameLine cannot drift them apart.
	const float32 rowY = ImGui::GetCursorPosY();

	for (int32 i = 0; i < count; ++i)
	{
		if (i > 0)
			ImGui::SameLine(0.0f, gap);

		ImGui::PushID(i);

		// Colored axis badge: click to reset this component.
		ImGui::SetCursorPosY(rowY + badgeOffset);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Button, axes[i].color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, axes[i].hovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, axes[i].color);

		// Kept from before the row's widgets touch it: with the axes locked together, what one axis was
		// and what it became is the ratio the other two follow.
		const float32 before = values[i];
		bool axisChanged = false;

		if (ImGui::Button(axes[i].name, buttonSize))
		{
			RecordSnapshot();
			values[i] = resetValue;
			axisChanged = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		// The field's own padding is narrowed: three decimals of a world coordinate is a long number in a
		// column that is a third of a row, and the padding is room the digits need more than the frame does.
		ImGui::SameLine(0.0f, gap);
		ImGui::SetCursorPosY(rowY);
		ImGui::SetNextItemWidth(dragWidth);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, style.FramePadding.y));

		if (ImGui::DragFloat("##value", &values[i], speed, 0.0f, 0.0f, "%.3f"))
			axisChanged = true;

		ImGui::PopStyleVar();
		if (ImGui::IsItemActivated()) BeginEdit();

		if (ApplyTypedExpression(ImGui::GetItemID(), values[i]))
			axisChanged = true;

		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

		if (axisChanged && uniform && *uniform)
		{
			// A scale of zero has no proportion to keep, so the axes simply meet where this one went.
			const float32 factor = (before != 0.0f) ? (values[i] / before) : 0.0f;

			for (int32 other = 0; other < count; ++other)
				if (other != i)
					values[other] = (factor != 0.0f) ? (values[other] * factor) : values[i];
		}

		changed |= axisChanged;
		ImGui::PopID();
	}

	// The padlock, then the revert arrow — in that order on every row, so the arrow is always the last
	// thing on the right.
	ImGui::SameLine(0.0f, kRowEndGap);
	ImGui::SetCursorPosY(rowY);
	AlignRowEndGlyph();

	if (uniform)
	{
		if (LockButton("##uniform", *uniform))
			*uniform = !*uniform;
	}
	else
	{
		ImGui::Dummy(ImVec2(RowEndSlot(), RowEndSlot()));
	}

	bool modified = false;
	for (int32 i = 0; i < count; ++i)
		modified |= (values[i] != resetValue);

	ImGui::SameLine(0.0f, kRowEndGap);
	ImGui::SetCursorPosY(rowY);
	AlignRowEndGlyph();

	if (ResetToDefaultButton("##reset", modified))
	{
		RecordSnapshot();
		for (int32 i = 0; i < count; ++i)
			values[i] = resetValue;
		changed = true;
	}

	ImGui::PopID();
	return changed;
}

bool EditorLayer::DrawTransformVector(const char* label, float* values, int count, float speed,
	float resetValue, const char* unit, bool* uniform, int axisBase)
{
	// A solid plate carries the axis colour at full strength — a translucent one only muddied it against
	// the dark field — with the letter in near-white so it reads on top.
	struct Axis { const char8* letter; ImU32 tint; ImU32 tag; };
	static const Axis axes[3] = {
		{ "X", IM_COL32(236, 236, 236, 255), IM_COL32(176, 64, 75, 255) },   // Red   #b0404b
		{ "Y", IM_COL32(236, 236, 236, 255), IM_COL32(73, 137, 72, 255) },   // Green #498948
		{ "Z", IM_COL32(236, 236, 236, 255), IM_COL32(55, 103, 175, 255) },  // Blue  #3767af
	};

	bool changed = false;
	const ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushID(label);

	// The field carries its unit in its own format, so the number and its "°" read as one thing. ImGui
	// trims the suffix when the field is typed into, so arithmetic and expressions still work.
	char format[16];
	std::snprintf(format, sizeof(format), "%%.3f%s%s", (unit && *unit) ? " " : "", unit ? unit : "");

	const float32 rowHeight = ImGui::GetFrameHeight();
	const float32 rowGap = 3.0f;
	const float32 groupHeight = count * rowHeight + (count - 1) * rowGap;

	// The tag: a small rounded plate coloured by its axis, the letter centred in it. It only names the
	// axis — resetting is the revert arrow's job — so it sits a hair inside the row and carries no click.
	constexpr float32 kTag = 22.0f;
	constexpr float32 kTagInset = 2.0f;
	const float32 tagGap = 6.0f;

	// Two glyphs are kept at the row's end — the padlock and the revert arrow — centred against the group.
	const float32 rightWidth = 2.0f * (RowEndSlot() + kRowEndGap);

	const ImVec2 groupOrigin = ImGui::GetCursorPos();
	const float32 fieldLeft = kVectorLabelWidth + kTag + tagGap;
	const float32 fieldWidth = ImMax(ImGui::GetContentRegionAvail().x - fieldLeft - rightWidth, 24.0f);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Every row wears its axis letter, coloured: X and Y for a Vector2, and the lone rotation wears Z,
	// the axis a 2D angle turns about (its caller passes axisBase 2). The letter index counts from that
	// base, so a scalar can name any single axis while the column stays true down the whole Transform.
	const bool tagged = true;

	for (int32 i = 0; i < count; ++i)
	{
		ImGui::PushID(i);

		const float32 rowY = groupOrigin.y + i * (rowHeight + rowGap);

		const float32 before = values[i];
		bool axisChanged = false;

		// The axis tag: a small coloured plate with the axis letter. A square a hair inside the row, so it
		// reads a touch smaller than the field, centred in its column. It only names the axis — it takes
		// no click, and it is the revert arrow that resets.
		const Axis& axis = axes[axisBase + i];
		if (tagged)
		{
			ImGui::SetCursorPos(ImVec2(kVectorLabelWidth, rowY));
			const ImVec2 slot = ImGui::GetCursorScreenPos();

			const float32 tagSide = rowHeight - 2.0f * kTagInset;
			const ImVec2 tagMin(slot.x + (kTag - tagSide) * 0.5f, slot.y + kTagInset);
			const ImVec2 tagMax(tagMin.x + tagSide, tagMin.y + tagSide);
			drawList->AddRectFilled(tagMin, tagMax, axis.tag, style.FrameRounding);

			const ImVec2 letterSize = ImGui::CalcTextSize(axis.letter);
			drawList->AddText(ImVec2(tagMin.x + (tagSide - letterSize.x) * 0.5f, tagMin.y + (tagSide - letterSize.y) * 0.5f),
				axis.tint, axis.letter);
		}

		// The field.
		ImGui::SetCursorPos(ImVec2(fieldLeft, rowY));
		ImGui::SetNextItemWidth(fieldWidth);

		if (ImGui::DragFloat("##value", &values[i], speed, 0.0f, 0.0f, format))
			axisChanged = true;

		if (ImGui::IsItemActivated()) BeginEdit();

		if (ApplyTypedExpression(ImGui::GetItemID(), values[i]))
			axisChanged = true;

		if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

		if (axisChanged && uniform && *uniform)
		{
			const float32 factor = (before != 0.0f) ? (values[i] / before) : 0.0f;

			for (int32 other = 0; other < count; ++other)
				if (other != i)
					values[other] = (factor != 0.0f) ? (values[other] * factor) : values[i];
		}

		changed |= axisChanged;
		ImGui::PopID();
	}

	// The property's name, centred down the group on the left.
	ImGui::SetCursorPos(ImVec2(0.0f, groupOrigin.y + (groupHeight - ImGui::GetTextLineHeight()) * 0.5f));
	ImGui::TextUnformatted(label);

	// The row-end glyphs, centred down the group on the right: the padlock (scale only), then the revert
	// arrow, which appears only while a value is off its default.
	bool modified = false;
	for (int32 i = 0; i < count; ++i)
		modified |= (values[i] != resetValue);

	const float32 glyphY = groupOrigin.y + (groupHeight - RowEndSlot()) * 0.5f;
	float32 glyphX = ImGui::GetContentRegionMax().x - rightWidth + kRowEndGap;

	if (uniform)
	{
		ImGui::SetCursorPos(ImVec2(glyphX, glyphY));
		if (LockButton("##uniform", *uniform))
			*uniform = !*uniform;
	}

	glyphX += RowEndSlot() + kRowEndGap;
	ImGui::SetCursorPos(ImVec2(glyphX, glyphY));

	if (ResetToDefaultButton("##reset", modified))
	{
		RecordSnapshot();
		for (int32 i = 0; i < count; ++i)
			values[i] = resetValue;
		changed = true;
	}

	// Leave the cursor below the whole group, so the next property does not climb into it.
	ImGui::SetCursorPos(ImVec2(groupOrigin.x, groupOrigin.y + groupHeight + style.ItemSpacing.y));

	ImGui::PopID();
	return changed;
}

void EditorLayer::ApplyReflectorToSelection(const std::string& typeName, Reflector& setter)
{
	if (typeName.empty())
		return;

	for (const auto& entity : mSelection)
		for (const auto& component : entity->GetComponents())
			if (component->GetTypeName() == typeName)
				component->Reflect(setter);
}

void EditorLayer::ApplyReflectedField(const std::string& typeName, const char8* field, float32 value)
{
	FieldSetter setter(field, value);
	ApplyReflectorToSelection(typeName, setter);
}

void EditorLayer::ApplyReflectedField(const std::string& typeName, const char8* field, int32 value)
{
	FieldSetter setter(field, value);
	ApplyReflectorToSelection(typeName, setter);
}

void EditorLayer::ApplyReflectedField(const std::string& typeName, const char8* field, bool value)
{
	FieldSetter setter(field, value);
	ApplyReflectorToSelection(typeName, setter);
}

void EditorLayer::ApplyReflectedField(const std::string& typeName, const char8* field, const std::string& value)
{
	FieldSetter setter(field, value);
	ApplyReflectorToSelection(typeName, setter);
}

void EditorLayer::ApplyReflectedField(const std::string& typeName, const char8* field, const Vector& value)
{
	FieldSetter setter(field, value);
	ApplyReflectorToSelection(typeName, setter);
}

void EditorLayer::InspectorReflector::Field(const char8* name, float32& value)
{
	mDrew = true;

	// No range and no default: the component said it was a number, not what a sensible one would be.
	if (mEditor.DrawFloatProperty(name, value, 0.1f, 0.0f, 0.0f, std::nullopt))
		mEditor.ApplyReflectedField(mTypeName, name, value);
}

void EditorLayer::InspectorReflector::Field(const char8* name, int32& value)
{
	mDrew = true;

	ImGui::PushID(name);
	PropertyLabel(name);

	if (ImGui::DragInt("##value", &value))
		mEditor.ApplyReflectedField(mTypeName, name, value);

	// An integer field takes arithmetic too; the answer lands on the nearest whole number.
	float32 typed = static_cast<float32>(value);

	if (ApplyTypedExpression(ImGui::GetItemID(), typed))
	{
		value = static_cast<int32>(std::lround(typed));
		mEditor.ApplyReflectedField(mTypeName, name, value);
	}

	if (ImGui::IsItemActivated()) mEditor.BeginEdit();
	if (ImGui::IsItemDeactivatedAfterEdit()) mEditor.CommitEdit();

	SameLineRowEnd();
	ResetToDefaultButton("##reset", false);   // Keeps the slot, so the row lines up with the ones that have an arrow.
	ImGui::PopID();
}

void EditorLayer::InspectorReflector::Field(const char8* name, bool& value)
{
	mDrew = true;

	ImGui::PushID(name);
	PropertyLabel(name);

	if (ImGui::Checkbox("##value", &value))
	{
		mEditor.RecordSnapshot();
		mEditor.ApplyReflectedField(mTypeName, name, value);
	}

	SameLineRowEnd();
	ResetToDefaultButton("##reset", false);
	ImGui::PopID();
}

void EditorLayer::InspectorReflector::Field(const char8* name, std::string& value)
{
	mDrew = true;

	char8 buffer[256];
	const size_t length = value.copy(buffer, sizeof(buffer) - 1);
	buffer[length] = '\0';

	ImGui::PushID(name);
	PropertyLabel(name);

	if (ImGui::InputText("##value", buffer, sizeof(buffer)))
		value = buffer;

	if (ImGui::IsItemActivated()) mEditor.BeginEdit();
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		mEditor.ApplyReflectedField(mTypeName, name, value);
		mEditor.CommitEdit();
	}

	SameLineRowEnd();
	ResetToDefaultButton("##reset", false);
	ImGui::PopID();
}

void EditorLayer::InspectorReflector::Field(const char8* name, Vector& value)
{
	mDrew = true;

	float32 values[3] = { value.x, value.y, value.z };

	if (mEditor.DrawVec3Control(name, values, 0.1f, 0.0f))
	{
		value = Vector(values[0], values[1], values[2]);
		mEditor.ApplyReflectedField(mTypeName, name, value);
	}
}

void EditorLayer::InspectorReflector::FieldAsset(const char8* name, std::string& path)
{
	mDrew = true;

	char8 buffer[256];
	const size_t length = path.copy(buffer, sizeof(buffer) - 1);
	buffer[length] = '\0';

	const ImGuiStyle& style = ImGui::GetStyle();
	const float32 browseWidth = ImGui::CalcTextSize(ICON_MDI_FOLDER_OPEN).x + style.FramePadding.x * 2.0f;

	ImGui::PushID(name);
	PropertyLabel(name);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseWidth - style.ItemInnerSpacing.x - RowEndSlot() - kRowEndGap);

	if (ImGui::InputText("##value", buffer, sizeof(buffer)))
		path = buffer;

	if (ImGui::IsItemActivated()) mEditor.BeginEdit();
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		mEditor.ApplyReflectedField(mTypeName, name, path);
		mEditor.CommitEdit();
	}

	// The same drop the Sprite Renderer takes, for the same reason: an asset is picked, never spelled.
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LN_ASSET_PATH"))
		{
			mEditor.RecordSnapshot();
			path = static_cast<const char8*>(payload->Data);
			mEditor.ApplyReflectedField(mTypeName, name, path);
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

	if (ImGui::Button(ICON_MDI_FOLDER_OPEN))
	{
		const std::string picked = FileDialog::Open("All Files\0*.*\0", GameAssetsDirectory().string());

		if (!picked.empty())
		{
			mEditor.RecordSnapshot();
			path = ToResourceRelativePath(picked);
			mEditor.ApplyReflectedField(mTypeName, name, path);
		}
	}

	SameLineRowEnd();
	ResetToDefaultButton("##reset", false);
	ImGui::PopID();
}

void EditorLayer::DrawProperties()
{
	if (!mShowProperties)
		return;

	ImGui::Begin(kPropertiesWindow, &mShowProperties);

	if (!mSelectedEntity)
	{
		ImGui::TextDisabled("Select an entity to edit its components.");
		ImGui::End();
		return;
	}

	// The same icon the Hierarchy draws, so the two panels agree about what they are pointing at — but larger
	// here, because this is the one entity the whole panel is about, not one row among many.
	DrawInlineIcon(mSelectedEntity->IsFolder() ? ICON_MDI_FOLDER : ICON_MDI_CUBE_OUTLINE,
		kIconTitle, ImGui::GetColorU32(ImGuiCol_Text));
	ImGui::SameLine();

	// Whether the entity is switched on at all — the checkbox Unity puts before the name, and the same
	// flag the game reads. Hiding is a different thing entirely, and lives on the eye in the Hierarchy.
	bool enabled = mSelectedEntity->IsEnabled();

	if (ImGui::Checkbox("##enabled", &enabled))
	{
		RecordSnapshot();

		for (const auto& entity : mSelection)
			entity->SetEnabled(enabled);
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(enabled ? "Enabled" : "Disabled — neither updated nor drawn");

	// Editable entity name.
	char nameBuffer[128];
	const std::string& currentName = mSelectedEntity->GetName();
	const size_t copied = currentName.copy(nameBuffer, sizeof(nameBuffer) - 1);
	nameBuffer[copied] = '\0';

	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);

	if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
		mSelectedEntity->SetName(nameBuffer);
	if (ImGui::IsItemActivated()) BeginEdit();
	if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

	ImGui::Separator();

	// The fields scroll; the summary below them does not. It is the same bargain the Hierarchy strikes: what
	// you are looking at is stated in one place, at the bottom, and it stays there however long the list is.
	ImGui::BeginChild("##components", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()));

	// A folder only organizes the hierarchy: it has no transform or components to edit.
	if (mSelectedEntity->IsFolder())
	{
		ImGui::TextDisabled("Folder — groups entities in the hierarchy.");
		EndPropertiesPanel();
		return;
	}

	bool transformRemove = false;
	int transformDrag = -1;
	if (DrawComponentHeader(ICON_MDI_AXIS_ARROW, "Transform", -1, transformRemove, transformDrag, transformDrag))
	{
		// Every entity has a Transform, so a transform edit is the one edit the whole selection always
		// has in common. The values shown are the primary's; the value written is written to all of them.
		const Reference<Transform> transform = mSelectedEntity->GetTransform();

		Vector2 position = transform->GetPosition();
		float32 positionValues[2] = { position.x, position.y };
		if (DrawTransformVector("Position", positionValues, 2, 1.0f, 0.0f, ""))
			for (const auto& entity : mSelection)
				entity->GetTransform()->SetPosition(Vector2(positionValues[0], positionValues[1]));

		// A rotation on a plane is one angle, not a vector whose X and Y meant nothing — one field, in
		// degrees, turning about Z, so it wears the Z tag (axis base 2).
		float32 rotationValue[1] = { transform->GetRotation() };
		if (DrawTransformVector("Rotation", rotationValue, 1, 0.5f, 0.0f, "\xC2\xB0", nullptr, 2))
			for (const auto& entity : mSelection)
				entity->GetTransform()->SetRotation(rotationValue[0]);

		Vector2 scale = transform->GetScale();
		float32 scaleValues[2] = { scale.x, scale.y };
		if (DrawTransformVector("Scale", scaleValues, 2, 0.01f, 1.0f, "", &mScaleUniform))
			for (const auto& entity : mSelection)
				entity->GetTransform()->SetScale(Vector2(scaleValues[0], scaleValues[1]));
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
			if (DrawComponentHeader(ICON_MDI_IMAGE, "Sprite Renderer", i, remove, dragFrom, dragTo))
			{
				// Reload the texture only once the user finishes editing (not per keystroke).
				char textureBuffer[256];
				const std::string& path = renderer->GetTexturePath();
				const size_t length = path.copy(textureBuffer, sizeof(textureBuffer) - 1);
				textureBuffer[length] = '\0';

				// The field gives up just enough room for the browse button at the end of the row.
				const ImGuiStyle& style = ImGui::GetStyle();
					const float32 browseWidth = ImGui::CalcTextSize(ICON_MDI_FOLDER_OPEN).x + style.FramePadding.x * 2.0f;

				PropertyLabel("Texture");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseWidth - style.ItemInnerSpacing.x);
				ImGui::InputText("##texture", textureBuffer, sizeof(textureBuffer));
				if (ImGui::IsItemActivated()) BeginEdit();
				if (ImGui::IsItemDeactivatedAfterEdit()) { ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetTexturePath(textureBuffer); }); CommitEdit(); }

				// Drop an image from the Project panel to assign it.
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LN_ASSET_PATH"))
					{
						RecordSnapshot();
						ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetTexturePath(static_cast<const char8*>(payload->Data)); });
					}

					ImGui::EndDragDropTarget();
				}

				// Browse for a sprite file; store the path relative to the resource root. The dialog opens
				// inside the project's assets, which is the only place a sprite can come from.
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				if (ImGui::Button(ICON_MDI_FOLDER_OPEN))
				{
					const std::string picked = FileDialog::Open(
						"Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0",
						GameAssetsDirectory().string());

					if (!picked.empty())
					{
						RecordSnapshot();
						ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetTexturePath(ToResourceRelativePath(picked)); });
					}
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Browse for a sprite image");

				// What it goes on top of. Zero means "wherever my row is" — the Hierarchy's order decides,
				// so moving a row down moves the sprite in front. A number overrides that row.
				int32 order = renderer->GetOrder();

				PropertyLabel("Draw Order");
				if (ImGui::DragInt("##order", &order))
					ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetOrder(order); });

				if (ImGui::IsItemActivated()) BeginEdit();

				float32 typedOrder = static_cast<float32>(order);

				if (ApplyTypedExpression(ImGui::GetItemID(), typedOrder))
				{
					order = static_cast<int32>(std::lround(typedOrder));
					ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetOrder(order); });
				}

				if (ImGui::IsItemDeactivatedAfterEdit()) CommitEdit();

				SameLineRowEnd();
				if (ResetToDefaultButton("##resetorder", order != 0))
				{
					RecordSnapshot();
					ApplyToSelection<SpriteRenderer>([](SpriteRenderer* target) { target->SetOrder(0); });
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Higher draws on top. 0 follows the Hierarchy's order.");

				// Mirroring, read out of the texture rather than taken out of the scale: a negative scale
				// would flip the collider and the children with it.
				bool flipX = renderer->IsFlippedX();
				bool flipY = renderer->IsFlippedY();

				PropertyLabel("Flip");
				if (ImGui::Checkbox("X##flipx", &flipX))
				{
					RecordSnapshot();
					ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetFlipX(flipX); });
				}

				ImGui::SameLine();

				if (ImGui::Checkbox("Y##flipy", &flipY))
				{
					RecordSnapshot();
					ApplyToSelection<SpriteRenderer>([&](SpriteRenderer* target) { target->SetFlipY(flipY); });
				}

				SameLineRowEnd();
				ResetToDefaultButton("##resetflip", false);   // Keeps the slot, so the row lines up.
			}
		}
		else if (Camera2D* camera = dynamic_cast<Camera2D*>(component))
		{
			if (DrawComponentHeader(ICON_MDI_MONITOR, "Camera 2D", i, remove, dragFrom, dragTo))
			{
				float32 zoom = camera->GetZoom();
				if (DrawFloatProperty("Zoom", zoom, 0.01f, 0.01f, 100.0f, 1.0f))
					ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetZoom(zoom); });

				Vector2 offset = camera->GetOffset();
				float32 offsetValues[2] = { offset.x, offset.y };

				if (DrawTransformVector("Offset", offsetValues, 2, 1.0f, 0.0f, "px"))
					ApplyToSelection<Camera2D>([&](Camera2D* target)
						{ target->SetOffset(Vector2(offsetValues[0], offsetValues[1])); });

				// The limit is one switch with four numbers under it — the sides of a level, which is how
				// a level is measured. Folded away until it is on, because four numbers that do nothing
				// are four numbers in the way.
				bool limit = camera->HasLimit();

				PropertyLabel("Limit");
				if (ImGui::Checkbox("##limit", &limit))
				{
					RecordSnapshot();
					ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetLimit(limit); });
				}

				SameLineRowEnd();
				ResetToDefaultButton("##resetlimit", false);

				if (limit)
				{
					ImGui::Indent();

					float32 top = camera->GetLimitTop();
					if (DrawFloatProperty("Top", top, 1.0f, -100000.0f, 100000.0f, std::nullopt))
						ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetLimitTop(top); });

					float32 right = camera->GetLimitRight();
					if (DrawFloatProperty("Right", right, 1.0f, -100000.0f, 100000.0f, std::nullopt))
						ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetLimitRight(right); });

					float32 bottom = camera->GetLimitBottom();
					if (DrawFloatProperty("Bottom", bottom, 1.0f, -100000.0f, 100000.0f, std::nullopt))
						ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetLimitBottom(bottom); });

					float32 left = camera->GetLimitLeft();
					if (DrawFloatProperty("Left", left, 1.0f, -100000.0f, 100000.0f, std::nullopt))
						ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetLimitLeft(left); });

					ImGui::Unindent();
				}

				// Smoothing, the same shape: a switch, and the speeds it eases at underneath.
				bool smooth = camera->HasSmoothing();

				PropertyLabel("Smooth");
				if (ImGui::Checkbox("##smooth", &smooth))
				{
					RecordSnapshot();
					ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetSmoothing(smooth); });
				}

				SameLineRowEnd();
				ResetToDefaultButton("##resetsmooth", false);

				if (smooth)
				{
					ImGui::Indent();

					float32 position = camera->GetPositionSmoothing();
					if (DrawFloatProperty("Position Speed", position, 0.1f, 0.0f, 100.0f, 5.0f))
						ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetPositionSmoothing(position); });

					float32 rotation = camera->GetRotationSmoothing();
					if (DrawFloatProperty("Rotation Speed", rotation, 0.1f, 0.0f, 100.0f, 5.0f))
						ApplyToSelection<Camera2D>([&](Camera2D* target) { target->SetRotationSmoothing(rotation); });

					ImGui::Unindent();
				}
			}
		}
		else if (RigidBody2D* body = dynamic_cast<RigidBody2D*>(component))
		{
			if (DrawComponentHeader(ICON_MDI_WEIGHT, "Rigid Body 2D", i, remove, dragFrom, dragTo))
			{
				static const char8* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
				const int32 defaultType = static_cast<int32>(BodyType::Dynamic);

				int32 typeIndex = static_cast<int32>(body->GetBodyType());
				PropertyLabel("Type");
				if (ImGui::Combo("##type", &typeIndex, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
				{
					RecordSnapshot();
					ApplyToSelection<RigidBody2D>([&](RigidBody2D* target) { target->SetBodyType(static_cast<BodyType>(typeIndex)); });
				}

				SameLineRowEnd();
				if (ResetToDefaultButton("##resetType", typeIndex != defaultType))
				{
					RecordSnapshot();
					ApplyToSelection<RigidBody2D>([](RigidBody2D* target) { target->SetBodyType(BodyType::Dynamic); });
				}

				bool fixedRotation = body->IsFixedRotation();
				PropertyLabel("Fixed Rotation");
				if (ImGui::Checkbox("##fixedRotation", &fixedRotation))
				{
					RecordSnapshot();
					ApplyToSelection<RigidBody2D>([&](RigidBody2D* target) { target->SetFixedRotation(fixedRotation); });
				}

				SameLineRowEnd();
				if (ResetToDefaultButton("##resetFixedRotation", fixedRotation))
				{
					RecordSnapshot();
					ApplyToSelection<RigidBody2D>([](RigidBody2D* target) { target->SetFixedRotation(false); });
				}
			}
		}
		else if (BoxCollider2D* collider = dynamic_cast<BoxCollider2D*>(component))
		{
			if (DrawComponentHeader(ICON_MDI_VECTOR_SQUARE, "Box Collider 2D", i, remove, dragFrom, dragTo))
			{
				// A collider's extents have no default worth going back to: a fresh one is zero-sized, and
				// what the editor gives it when you add it is the sprite's size, not something the class
				// knows. So they carry no arrow — one that reverted a box to nothing would be a trap.
				float32 width = collider->GetWidth();
				if (DrawFloatProperty("Width", width, 1.0f, 0.0f, 10000.0f, std::nullopt)) ApplyToSelection<BoxCollider2D>([&](BoxCollider2D* target) { target->SetWidth(width); });

				float32 height = collider->GetHeight();
				if (DrawFloatProperty("Height", height, 1.0f, 0.0f, 10000.0f, std::nullopt)) ApplyToSelection<BoxCollider2D>([&](BoxCollider2D* target) { target->SetHeight(height); });

				float32 density = collider->GetDensity();
				if (DrawFloatProperty("Density", density, 0.05f, 0.0f, 100.0f, 1.0f)) ApplyToSelection<BoxCollider2D>([&](BoxCollider2D* target) { target->SetDensity(density); });

				float32 friction = collider->GetFriction();
				if (DrawFloatProperty("Friction", friction, 0.01f, 0.0f, 1.0f, 0.2f)) ApplyToSelection<BoxCollider2D>([&](BoxCollider2D* target) { target->SetFriction(friction); });

				float32 restitution = collider->GetRestitution();
				if (DrawFloatProperty("Restitution", restitution, 0.01f, 0.0f, 1.0f, 0.0f)) ApplyToSelection<BoxCollider2D>([&](BoxCollider2D* target) { target->SetRestitution(restitution); });
			}
		}
		else if (CircleCollider2D* collider = dynamic_cast<CircleCollider2D*>(component))
		{
			if (DrawComponentHeader(ICON_MDI_VECTOR_CIRCLE_VARIANT, "Circle Collider 2D", i, remove, dragFrom, dragTo))
			{
				// The radius has no default to go back to, for the same reason a box's extents do not.
				float32 radius = collider->GetRadius();
				if (DrawFloatProperty("Radius", radius, 1.0f, 0.0f, 10000.0f, std::nullopt)) ApplyToSelection<CircleCollider2D>([&](CircleCollider2D* target) { target->SetRadius(radius); });

				float32 density = collider->GetDensity();
				if (DrawFloatProperty("Density", density, 0.05f, 0.0f, 100.0f, 1.0f)) ApplyToSelection<CircleCollider2D>([&](CircleCollider2D* target) { target->SetDensity(density); });

				float32 friction = collider->GetFriction();
				if (DrawFloatProperty("Friction", friction, 0.01f, 0.0f, 1.0f, 0.2f)) ApplyToSelection<CircleCollider2D>([&](CircleCollider2D* target) { target->SetFriction(friction); });

				float32 restitution = collider->GetRestitution();
				if (DrawFloatProperty("Restitution", restitution, 0.01f, 0.0f, 1.0f, 0.0f)) ApplyToSelection<CircleCollider2D>([&](CircleCollider2D* target) { target->SetRestitution(restitution); });
			}
		}
		else
		{
			// Any other component — a user-defined one from the game module. The editor was never compiled
			// against it and has no idea what it holds, so it asks: the component describes its fields, and
			// the Inspector draws exactly what it was told about, no more.
			const std::string& typeName = component->GetTypeName();
			const char8* componentName = typeName.empty() ? "Component" : typeName.c_str();

			if (DrawComponentHeader(ICON_MDI_PUZZLE, componentName, i, remove, dragFrom, dragTo))
			{
				InspectorReflector reflector(*this, typeName);
				component->Reflect(reflector);

				if (!reflector.DrewAnything())
					ImGui::TextDisabled("No fields — describe them in Reflect().");
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
		// A menu item is shown while any entity in the selection lacks the component: an entity that has one
		// already is passed over, so adding it to five is adding it to the four that were missing it. The
		// list stops offering it only once every one of them has it. Adding to the whole selection, not to
		// the last one clicked, is the whole point of having selected more than one.
		const auto selectionLacks = [&](const auto& has)
		{
			for (const auto& entity : mSelection)
				if (!entity->IsFolder() && !has(entity))
					return true;
			return false;
		};

		const auto lacksBuiltIn = [&]<typename T>()
		{
			return selectionLacks([](const Reference<Entity>& e) { return e->HasComponent<T>(); });
		};

		if (lacksBuiltIn.operator()<SpriteRenderer>() && ImGui::MenuItem("Sprite Renderer"))
		{
			RecordSnapshot();
			for (const auto& entity : mSelection)
				if (!entity->IsFolder() && !entity->HasComponent<SpriteRenderer>())
					entity->AddComponent<SpriteRenderer>();
		}

		if (lacksBuiltIn.operator()<Camera2D>() && ImGui::MenuItem("Camera 2D"))
		{
			RecordSnapshot();
			for (const auto& entity : mSelection)
				if (!entity->IsFolder() && !entity->HasComponent<Camera2D>())
					entity->AddComponent<Camera2D>();
		}

		if (lacksBuiltIn.operator()<RigidBody2D>() && ImGui::MenuItem("Rigid Body 2D"))
		{
			RecordSnapshot();
			for (const auto& entity : mSelection)
				if (!entity->IsFolder() && !entity->HasComponent<RigidBody2D>())
					entity->AddComponent<RigidBody2D>();
		}

		// A new collider fits its own entity's sprite (Unity-style): the size is read per entity, not from
		// the primary, so each one ends up with the box its own picture needs. Sizes are unscaled, so the
		// Transform scale applies on top.
		if (lacksBuiltIn.operator()<BoxCollider2D>() && ImGui::MenuItem("Box Collider 2D"))
		{
			RecordSnapshot();
			for (const auto& entity : mSelection)
			{
				if (entity->IsFolder() || entity->HasComponent<BoxCollider2D>())
					continue;

				const SpriteRenderer* sprite = entity->GetComponent<SpriteRenderer>();
				const Size size = sprite ? sprite->GetSize() : Size(100.0f, 100.0f);
				entity->AddComponent<BoxCollider2D>(size.width, size.height);
			}
		}

		if (lacksBuiltIn.operator()<CircleCollider2D>() && ImGui::MenuItem("Circle Collider 2D"))
		{
			RecordSnapshot();
			for (const auto& entity : mSelection)
			{
				if (entity->IsFolder() || entity->HasComponent<CircleCollider2D>())
					continue;

				const SpriteRenderer* sprite = entity->GetComponent<SpriteRenderer>();
				const Size size = sprite ? sprite->GetSize() : Size(100.0f, 100.0f);
				entity->AddComponent<CircleCollider2D>(std::max(size.width, size.height) * 0.5f);
			}
		}

		// Components coming from the loaded game module (user-defined ones). They have no special
		// construction here — created by name, default-constructed, then configured in the Inspector.
		// The built-in types above are listed explicitly, so they are skipped here.
		bool sawGameComponent = false;

		for (const std::string& name : ComponentRegistry::GetNames())
		{
			if (IsBuiltInComponent(name))
				continue;

			if (!selectionLacks([&](const Reference<Entity>& e) { return e->HasComponentByName(name); }))
				continue;

			if (!sawGameComponent)
			{
				ImGui::Separator();
				sawGameComponent = true;
			}

			if (ImGui::MenuItem(name.c_str()))
			{
				RecordSnapshot();
				for (const auto& entity : mSelection)
					if (!entity->IsFolder() && !entity->HasComponentByName(name))
						entity->AddComponentByName(name);
			}
		}

		// Scaffold a brand new component class into the game's source tree, Unreal-style.
		ImGui::Separator();

		if (ImGui::MenuItem("New C++ Component..."))
			mOpenNewComponentPopup = true;

		ImGui::EndPopup();
	}

	EndPropertiesPanel();
}

void EditorLayer::EndPropertiesPanel()
{
	ImGui::EndChild();
	ImGui::Separator();

	const int32 count = static_cast<int32>(mSelection.size());

	// One entity needs no explaining. Several do: the fields show what the last one clicked has, and write
	// to every one of them that has the same field to write.
	if (count > 1)
		ImGui::TextDisabled("%d Entities | Edits apply to all of them.", count);
	else
		ImGui::TextDisabled("1 Entity");

	ImGui::End();
}

void EditorLayer::DrawStatusBar()
{
	// A bar along the bottom of the viewport, which is what makes the dockspace above it stop short —
	// ImGui takes it out of the work area, so no panel has to know the bar is there.
	ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;

	if (!ImGui::BeginViewportSideBar("LionEditorStatusBar", ImGui::GetMainViewport(), ImGuiDir_Down,
		ImGui::GetFrameHeight(), flags))
	{
		ImGui::End();
		return;
	}

	if (ImGui::BeginMenuBar())
	{
		// What is open moved to the title bar, where what the window has open belongs. What is left is the
		// version, on the right, with room to breathe: text against the edge of the screen reads as clipped.
		// The bar has space for more; this is what it starts with.
		//
		// Placed, not aligned: SameLine puts an item next to the one before it, and there is nothing before
		// this one — it took its vertical position from whatever the last window happened to draw, which is
		// how the version ended up off the bar entirely.
		constexpr float32 kEdge = 8.0f;
		const float32 versionWidth = ImGui::CalcTextSize(kVersion).x;

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - versionWidth - kEdge);
		ImGui::TextDisabled("%s", kVersion);

		ImGui::EndMenuBar();
	}

	ImGui::End();
}

void EditorLayer::DrawTitleBar()
{
	// The window has no caption of its own, so this is it, and it is two rows tall.
	//
	// The top row is the window's: the menus and the buttons that minimise, maximise and close it — the
	// things a caption is *for*, kept where a caption keeps them. The bottom row is the editor's: what is
	// open, which is the question the title used to answer badly.
	//
	// It is a side bar, like the status bar, so it comes out of the viewport's work area and the dockspace
	// below it starts where it ends.
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	const bool open = ImGui::BeginViewportSideBar("LionEditorTitleBar", viewport, ImGuiDir_Up, kTitleBarHeight, flags);
	ImGui::PopStyleVar(2);

	if (!open)
	{
		ImGui::End();
		return;
	}

	const ImVec2 barMin = ImGui::GetWindowPos();
	const float32 barWidth = ImGui::GetWindowWidth();
	const float32 row = kTitleBarHeight * 0.5f;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(barMin, ImVec2(barMin.x + barWidth, barMin.y + kTitleBarHeight),
		ImGui::GetColorU32(ImGuiCol_MenuBarBg));

	// The mark spans both rows, which is the whole reason there are two of them.
	constexpr float32 kMark = 34.0f;
	constexpr float32 kMarkLeft = 12.0f;

	if (mLogo)
		drawList->AddImage(
			static_cast<ImTextureID>(mLogo->GetNativeHandle()),
			ImVec2(barMin.x + kMarkLeft, barMin.y + (kTitleBarHeight - kMark) * 0.5f),
			ImVec2(barMin.x + kMarkLeft + kMark, barMin.y + (kTitleBarHeight + kMark) * 0.5f),
			ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));   // Bottom-up, as OpenGL loaded it.

	const float32 contentLeft = kMarkLeft + kMark + 14.0f;
	const float32 menuHeight = ImGui::GetFrameHeight();

	// --- Top row: the menus, the project, and the window's own buttons at the far end.
	//
	// The project sits with the buttons because that is what the window *is* — the thing you closed when you
	// close it — while the scene sits under the menus, because it is what the menus act on.
	const float32 menusEnd = DrawMenuBar(
		ImVec2(barMin.x + contentLeft, barMin.y + (row - menuHeight) * 0.5f),
		ImVec2(barMin.x + barWidth, barMin.y + (row + menuHeight) * 0.5f));

	DrawWindowButtons(barMin, barWidth, row);

	// The editor's own name, in the middle of the row the window's controls are in — which is where a window
	// says what it is, whether or not it drew the caption itself. Bold, so it reads as the name of the thing
	// and not as one more label in a bar full of them; measured under the same font it is drawn in.
	ImGui::PushFont(EditorGui::GetBoldFont());

	const float32 titleWidth = ImGui::CalcTextSize(kEditorName).x;

	ImGui::SetCursorPos(ImVec2((barWidth - titleWidth) * 0.5f, (row - ImGui::GetTextLineHeight()) * 0.5f));
	ImGui::TextUnformatted(kEditorName);

	ImGui::PopFont();

	const std::filesystem::path project = ActiveProjectDirectory();
	const std::string projectName = project.empty() ? std::string("No project") : ProjectDisplayName(project);

	const float32 projectWidth = ImGui::CalcTextSize(projectName.c_str()).x;
	const float32 projectLeft = barWidth - kWindowButton * 3.0f - kProjectGap - projectWidth;

	// The project sits in a tab of its own, hanging from the top edge of the window: square where it meets the
	// edge and round where it leaves it, which is what makes it read as hanging from the edge rather than
	// floating in front of it.
	//
	// The fill alone was nearly the colour of the bar behind it, so the tab had no edge and the name looked
	// like it was floating; the outline is what says where the tab stops.
	const ImVec2 boxMin(barMin.x + projectLeft - kProjectPadding, barMin.y);
	const ImVec2 boxMax(barMin.x + projectLeft + projectWidth + kProjectPadding, barMin.y + row);

	// The tab opens the Project Manager: an invisible button over the whole box catches the click, and its
	// hover lights the fill so it reads as something to press rather than a label.
	ImGui::SetCursorScreenPos(boxMin);
	if (ImGui::InvisibleButton("##project_chip", ImVec2(boxMax.x - boxMin.x, boxMax.y - boxMin.y)))
		mOpenProjectManagerPopup = true;

	const bool projectHovered = ImGui::IsItemHovered();
	const ImU32 fill = projectHovered ? IM_COL32(0x3C, 0x3C, 0x40, 255) : IM_COL32(0x30, 0x30, 0x33, 255);

	drawList->AddRectFilled(boxMin, boxMax, fill, kProjectRounding, ImDrawFlags_RoundCornersBottom);
	drawList->AddRect(boxMin, boxMax, IM_COL32(0x4A, 0x4A, 0x4F, 255), kProjectRounding, ImDrawFlags_RoundCornersBottom, 1.0f);

	ImGui::SetCursorPos(ImVec2(projectLeft, (row - ImGui::GetTextLineHeight()) * 0.5f));
	ImGui::TextUnformatted(projectName.c_str());

	if (projectHovered)
		ImGui::SetTooltip("%s\nClick to manage projects", project.empty() ? "No project open" : project.generic_string().c_str());

	// --- Bottom row: the scene open in the project, picking up where the menus left off. It used to be named
	// in the Hierarchy, which is a panel that shows a scene, not the thing that has one open.
	const std::string sceneName = mScenePath.empty()
		? std::string("Untitled")
		: std::filesystem::path(mScenePath).stem().generic_string();

	ImGui::SetCursorPos(ImVec2(menusEnd - barMin.x + kSceneGap, row + (row - ImGui::GetTextLineHeight()) * 0.5f));
	ImGui::TextDisabled("|");
	ImGui::SameLine(0.0f, 8.0f);
	ImGui::TextDisabled("%s", sceneName.c_str());

	if (!mScenePath.empty() && ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", mScenePath.c_str());

	// What is left of the bar is what the window is dragged by; what the editor drew in it is not. A drag
	// that began on a menu would open nothing and move everything.
	const bool overBar = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
	Window::SetTitleBarBlocked(!overBar || ImGui::IsAnyItemHovered());

	ImGui::End();
}

void EditorLayer::DrawWindowButtons(const ImVec2& barMin, float32 barWidth, float32 rowHeight)
{
	// Minimise, maximise and close, drawn the way the rest of the editor's glyphs are: the font carries
	// none of them, and neither does the window anymore.
	constexpr float32 kArm = 5.0f;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for (int32 kind = 0; kind < 3; ++kind)
	{
		const float32 left = barMin.x + barWidth - kWindowButton * (3 - kind);

		ImGui::PushID(kind);
		ImGui::SetCursorScreenPos(ImVec2(left, barMin.y));

		const bool pressed = ImGui::InvisibleButton("##button", ImVec2(kWindowButton, rowHeight));
		const bool hovered = ImGui::IsItemHovered();

		ImGui::PopID();

		// Hovered, each button lights in its own colour, the way a traffic light does on a Mac: minimise green,
		// maximise amber, close red. The close red was already right; the other two join it.
		static const ImU32 kHoverFill[3] = {
			IM_COL32(46, 160, 67, 255),   // Minimise.
			IM_COL32(219, 158, 32, 255),  // Maximise.
			IM_COL32(196, 43, 28, 255),   // Close.
		};

		if (hovered)
			drawList->AddRectFilled(ImVec2(left, barMin.y), ImVec2(left + kWindowButton, barMin.y + rowHeight),
				kHoverFill[kind]);

		const ImVec2 center(left + kWindowButton * 0.5f, barMin.y + rowHeight * 0.5f);
		const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);

		switch (kind)
		{
			case 0:
				drawList->AddLine(ImVec2(center.x - kArm, center.y), ImVec2(center.x + kArm, center.y), color, 1.0f);
				break;

			case 1:
			{
				// The button says what pressing it will do, and what it will do depends on where the window
				// already is: one square makes it fill the screen, two say it will step back off it.
				//
				// The two sit on a smaller square than the one does: a pair set apart from each other reads
				// heavier than a single square of the same size, and the three buttons must weigh the same.
				const bool maximized = Window::IsMaximized();
				const float32 offset = maximized ? 2.0f : 0.0f;
				const float32 arm = maximized ? kArm - 1.5f : kArm - 0.5f;

				if (maximized)
					drawList->AddRect(
						ImVec2(center.x - arm + offset, center.y - arm - offset),
						ImVec2(center.x + arm + offset, center.y + arm - offset),
						color, 0.0f, 0, 1.0f);

				// The front square is filled with whatever is behind it, so the square behind it stops at the
				// front one's edge instead of showing through. Behind it is the bar — or, while the button is
				// hovered, the amber that fills the whole button. Filling with the bar's colour then left a
				// bar-coloured hole in the amber, which is the square that "was not painted inside".
				const ImVec2 frontMin(center.x - arm - offset, center.y - arm + offset);
				const ImVec2 frontMax(center.x + arm - offset, center.y + arm + offset);

				if (maximized)
					drawList->AddRectFilled(frontMin, frontMax,
						hovered ? kHoverFill[1] : ImGui::GetColorU32(ImGuiCol_MenuBarBg));

				drawList->AddRect(frontMin, frontMax, color, 0.0f, 0, 1.0f);

				break;
			}

			case 2:
				drawList->AddLine(ImVec2(center.x - kArm, center.y - kArm), ImVec2(center.x + kArm, center.y + kArm), color, 1.2f);
				drawList->AddLine(ImVec2(center.x - kArm, center.y + kArm), ImVec2(center.x + kArm, center.y - kArm), color, 1.2f);
				break;
		}

		if (!pressed)
			continue;

		switch (kind)
		{
			case 0: Window::Minimize();       break;
			case 1: Window::ToggleMaximize(); break;
			case 2: Window::RequestClose();   break;
		}
	}
}

void EditorLayer::NewScene()
{
	RecordSnapshot();
	mScene->Clear();
	SetSelection(nullptr);
	mScenePath.clear();
}

void EditorLayer::OpenScene()
{
	const std::string path = FileDialog::Open(kSceneFilter, GameAssetsDirectory().string());

	if (!path.empty())
		LoadScene(path);
}

bool EditorLayer::LoadScene(const std::string& path)
{
	RecordSnapshot();
	SetSelection(nullptr);

	if (!SceneSerializer::Deserialize(mScene, path))
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not open '{}'.", path));
		return false;
	}

	mScenePath = path;
	RememberRecentScene(path);
	return true;
}

void EditorLayer::SaveScene()
{
	// A scene that has never been anywhere has to be told where to go, so this is Save As until it is not.
	if (mScenePath.empty())
	{
		SaveSceneAs();
		return;
	}

	SceneSerializer::Serialize(mScene, mScenePath);
}

void EditorLayer::SaveSceneAs()
{
	const std::string path = FileDialog::Save(kSceneFilter, "lnscene", GameAssetsDirectory().string());

	if (path.empty())
		return;

	SceneSerializer::Serialize(mScene, path);
	mScenePath = path;
	RememberRecentScene(path);
}

float32 EditorLayer::DrawMenuBar(const ImVec2& barMin, const ImVec2& barMax)
{
	if (!BeginMenuBarAt(barMin, barMax))
		return barMin.x;

	{
		if (ImGui::BeginMenu("File"))
		{
			// The menu says what the key does and the key does what the menu does, because both call the one
			// function that does it — a rule written down twice is a rule that will disagree with itself.
			if (ImGui::MenuItem(ICON_MDI_FILE_PLUS_OUTLINE "  New Scene", ShortcutText(ShortcutAction::NewScene).c_str()))
				NewScene();

			if (ImGui::MenuItem(ICON_MDI_FOLDER_OPEN_OUTLINE "  Open Scene...", ShortcutText(ShortcutAction::OpenScene).c_str()))
				OpenScene();

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_MDI_CONTENT_SAVE_OUTLINE "  Save Scene", ShortcutText(ShortcutAction::SaveScene).c_str()))
				SaveScene();

			if (ImGui::MenuItem(ICON_MDI_CONTENT_SAVE_ALL_OUTLINE "  Save Scene As...", ShortcutText(ShortcutAction::SaveSceneAs).c_str()))
				SaveSceneAs();

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_MDI_FOLDER_MULTIPLE "  New Project...", ShortcutText(ShortcutAction::NewProject).c_str()))
				mOpenProjectManagerPopup = true;

			if (ImGui::MenuItem(ICON_MDI_FOLDER_OPEN "  Open Project...", ShortcutText(ShortcutAction::OpenProject).c_str()))
				BrowseForProject();

			ImGui::Separator();

			// What was recently in hand, one click from being in hand again. The menus stay in the frame
			// they were drawn in, so a chosen entry is applied after they close.
			std::string sceneToLoad;
			std::string projectToOpen;

			if (ImGui::BeginMenu(ICON_MDI_FILE_DOCUMENT_OUTLINE "  Recent Scenes"))
			{
				if (mRecentScenes.empty())
					ImGui::MenuItem("Nothing yet", nullptr, false, false);

				for (const std::string& path : mRecentScenes)
				{
					ImGui::PushID(path.c_str());

					if (ImGui::MenuItem(std::filesystem::path(path).stem().generic_string().c_str()))
						sceneToLoad = path;

					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", path.c_str());

					ImGui::PopID();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(ICON_MDI_FOLDER "  Recent Projects"))
			{
				if (mRecentProjects.empty())
					ImGui::MenuItem("Nothing yet", nullptr, false, false);

				for (const std::string& path : mRecentProjects)
				{
					ImGui::PushID(path.c_str());

					if (ImGui::MenuItem(ProjectDisplayName(path).c_str()))
						projectToOpen = path;

					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", path.c_str());

					ImGui::PopID();
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_MDI_EXIT_TO_APP "  Exit", "Alt+F4"))
				Window::RequestClose();

			ImGui::EndMenu();

			if (!sceneToLoad.empty())
				LoadScene(sceneToLoad);

			if (!projectToOpen.empty())
				OpenProject(projectToOpen);
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem(ICON_MDI_UNDO "  Undo", "Ctrl+Z", false, !mUndoStack.empty()))
				Undo();

			if (ImGui::MenuItem(ICON_MDI_REDO "  Redo", "Ctrl+Y", false, !mRedoStack.empty()))
				Redo();

			ImGui::EndMenu();
		}

		// Which panels are on screen. A real checkbox rather than ImGui's bare tick, so a hidden panel
		// still reads as a box waiting to be clicked instead of an empty row.
		if (ImGui::BeginMenu("View"))
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			float32 labelWidth = 0.0f;
			float32 shortcutWidth = 0.0f;

			for (const Panel& panel : kPanels)
			{
				const std::string shortcut = KeybindToString(mBinds[static_cast<int>(panel.shortcut)]);
				labelWidth = ImMax(labelWidth, ImGui::CalcTextSize(panel.label).x);
				shortcutWidth = ImMax(shortcutWidth, ImGui::CalcTextSize(shortcut.c_str()).x);
			}

			// Room for the checkbox, the longest label and the longest shortcut, so the column lines up.
			const float32 checkboxWidth = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x;

			for (const Panel& panel : kPanels)
			{
				ImGui::Checkbox(panel.label, &(this->*panel.visible));

				ImGui::SameLine(checkboxWidth + labelWidth + style.ItemSpacing.x * 3.0f);
				ImGui::TextDisabled("%s", KeybindToString(mBinds[static_cast<int>(panel.shortcut)]).c_str());
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Game"))
		{
			if (ImGui::MenuItem(ICON_MDI_HAMMER_WRENCH "  Compile", ShortcutText(ShortcutAction::CompileModule).c_str(), false, !mBuilding))
				CompileGameModule();

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Rebuild the game module and reload it");

			if (ImGui::MenuItem(ICON_MDI_RELOAD "  Reload Module", KeybindToString(mBinds[static_cast<int>(ShortcutAction::ReloadModule)]).c_str(), false, !mBuilding))
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
			ImGui::MenuItem(ICON_MDI_KEYBOARD "  Shortcuts", ShortcutText(ShortcutAction::ToggleShortcuts).c_str(), &mShowShortcuts);
			ImGui::EndMenu();
		}

		// Where the menus ran out, which is where the caller carries on: the row below begins under the end
		// of this one. Read before the bar is closed, because closing it puts the cursor back.
		const float32 menusEnd = ImGui::GetCurrentWindow()->DC.CursorPos.x;

		// A build says so in the toast, which is where it is said properly. It used to say so here as well,
		// in yellow, which is one thing being reported twice.
		EndMenuBarAt();

		return menusEnd;
	}
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceId)
{
	// Three columns: the browsers on the left, the viewport over the console in the middle, and the
	// inspectors on the right. Panel sizes are authored as round pixel counts rather than as
	// free-floating ratios, so at the usual window sizes the layout lands on the same even grid as
	// the rest of the UI.
	constexpr float32 kLeftWidth           = 264.0f;  // Scene Hierarchy over the Content Browser.
	constexpr float32 kRightWidth          = 372.0f;  // Statistics over Properties: the inspectors need the extra room.
	constexpr float32 kConsoleHeight       = 206.0f;
	constexpr float32 kContentBrowserHeight = 274.0f;
	constexpr float32 kStatisticsHeight    = 226.0f;

	const ImVec2 work = ImGui::GetMainViewport()->WorkSize;
	const float32 separator = ImGui::GetStyle().DockingSeparatorSize;

	// A split ratio is a fraction of what the two sides actually share, and the separator between them
	// belongs to neither — so it comes off the extent before the division. Measured against the whole
	// node instead, a panel rounds down by a pixel, and the pixel it gives up lands in the viewport:
	// the one panel whose size has to be exact, because the game is rendered at it.
	const auto ratio = [separator](float32 size, float32 extent)
	{
		return ImClamp(size / ImMax(extent - separator, 1.0f), 0.05f, 0.5f);
	};

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, work);

	// Each split measures against what the one before it left behind — its panel and its separator both.
	ImGuiID center = dockspaceId;
	ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, ratio(kRightWidth, work.x), nullptr, &center);
	ImGuiID left  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, ratio(kLeftWidth, work.x - kRightWidth - separator), nullptr, &center);

	const ImGuiID bottom     = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, ratio(kConsoleHeight, work.y), nullptr, &center);
	const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, ratio(kContentBrowserHeight, work.y), nullptr, &left);
	const ImGuiID rightTop   = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, ratio(kStatisticsHeight, work.y), nullptr, &right);

	ImGui::DockBuilderDockWindow(kHierarchyWindow, left);
	ImGui::DockBuilderDockWindow(kProjectWindow, leftBottom);
	ImGui::DockBuilderDockWindow(kViewportWindow, center);
	ImGui::DockBuilderDockWindow(kConsoleWindow, bottom);
	ImGui::DockBuilderDockWindow(kStatisticsWindow, rightTop);
	ImGui::DockBuilderDockWindow(kPropertiesWindow, right);

	ImGui::DockBuilderFinish(dockspaceId);
}

std::string EditorLayer::LayoutPath(const std::string& name)
{
	return (EditorLayoutsDirectory() / (name + ".ini")).string();
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

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(EditorLayoutsDirectory(), error))
		if (entry.is_regular_file() && entry.path().extension() == ".ini")
			layouts.push_back(entry.path().stem().string());

	std::sort(layouts.begin(), layouts.end());
	return layouts;
}

void EditorLayer::SaveLayout(const std::string& name) const
{
	std::error_code error;
	std::filesystem::create_directories(EditorLayoutsDirectory(), error);

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

	if (ImGui::MenuItem(ICON_MDI_VIEW_DASHBOARD_OUTLINE "  Default"))
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

	if (ImGui::MenuItem(ICON_MDI_CONTENT_SAVE_OUTLINE "  Save Layout..."))
		mOpenSaveLayoutPopup = true;

	if (ImGui::BeginMenu(ICON_MDI_DELETE_OUTLINE "  Delete Layout", !layouts.empty()))
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
	// Anchored to the executable, not to the working directory: the module sits with the editor's own
	// binaries, and the editor is not always started from that folder (Visual Studio runs it from the
	// project directory, and a shortcut can point anywhere). The *copy* goes to the Data folder, which
	// is the one place guaranteed writable: an editor installed where Windows guards the folder can
	// read the module beside itself but not put a file there — the "Access is denied" every launch.
	const std::filesystem::path root = ResourceRootDirectory();
	const std::filesystem::path data = EditorDataDirectory();
	const std::filesystem::path runtime = data / kGameModuleLoadedFile;

	std::error_code error;

	// The module belongs to the project when the project has built one — its own Build output, tied to
	// the SDK. The built-in, and a project that has never compiled, run the module beside the editor.
	const std::filesystem::path active = ActiveProjectDirectory();
	const std::filesystem::path owned = active.empty()
		? std::filesystem::path() : ProjectBuild::ModulePath(active);

	const std::filesystem::path source = (!owned.empty() && std::filesystem::exists(owned, error))
		? owned : root / kGameModuleFile;

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

	CopyGameSymbols(source.parent_path(), data);

	const bool loaded = Lion::LoadGameModule(mGameModule, runtime.string());

	if (loaded)
		Log::Console(LogLevel::Success, "[Editor] Loaded the game module.");

	return loaded;
}

void EditorLayer::CompileGameModule()
{
	if (mBuilding)
		return;

	if (MSBuildPath().empty())
	{
		Log::Console(LogLevel::Error,
			"[Editor] Could not locate MSBuild; building C++ needs Visual Studio with its C++ tools installed.");
		return;
	}

	const std::filesystem::path active = ActiveProjectDirectory();
	const bool builtIn = !active.empty()
		&& active.lexically_normal() == Projects::DefaultProjectDirectory().lexically_normal();

	std::string generate;
	std::string build;

	if (builtIn)
	{
		// The built-in Sandbox is part of the engine's own tree and builds the way the tree does:
		// premake regenerates (a file just scaffolded is not in the .vcxproj yet — the lists are globs),
		// then MSBuild builds only the module. Its target is its solution folder and project name, hence
		// "Runtime\Game"; BuildProjectReferences=false keeps the engine out of it, whose .pdb this very
		// editor's debugger may hold locked.
		const std::filesystem::path root = ProjectRootDirectory();

		if (root.empty())
		{
			Log::Console(LogLevel::Warning,
				"[Editor] The built-in project only exists inside the engine's source tree; there is nothing to compile here.");
			return;
		}

		generate = "cd /d \"" + root.string() + "\" && premake5 vs2022";

		build =
			"\"" + MSBuildPath() + "\""
			" \"" + (root / "Lion.sln").string() + "\""
			" -t:Runtime\\Game"
			" -p:BuildProjectReferences=false"
			" -p:Configuration=" + BuildConfiguration() +
			" -p:Platform=x64 -v:minimal -nologo";
	}
	else
	{
		// Any other project owns its build, the way an Unreal game does: a Visual Studio project of its
		// own, tied to the SDK beside the editor — which is what lets a distributed editor compile C++
		// with no engine tree in sight. Regenerated now, so the file list is the project as it stands.
		std::string error;

		if (!ProjectBuild::Generate(active, error))
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not prepare the project's build: {}", error));
			PushToast("Could not prepare the project's build", false);
			return;
		}

		build =
			"\"" + MSBuildPath() + "\""
			" \"" + ProjectBuild::VcxprojPath(active).string() + "\""
			" -p:Configuration=" + BuildConfiguration() +
			" -p:Platform=x64 -v:minimal -nologo";
	}

	Log::Console(LogLevel::Information, "[Editor] Compiling the game module...");
	PushToast("Compiling the game module", true);

	mBuilding = true;
	mGameBuild = std::async(std::launch::async, [generate, build]
	{
		GameBuild result;

		// A failed regeneration means the build would compile a stale file list, so it stops here. A
		// project's own build has no generate step: its file list was written just above.
		if (!generate.empty())
		{
			result.exitCode = RunCommand(generate, result.output);

			if (result.exitCode != 0)
			{
				result.output += "[Editor] Could not regenerate the projects; is premake5 on your PATH?\n";
				return result;
			}
		}

		result.exitCode = RunCommand(build, result.output);
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
		DismissBusyToasts();
		PushToast("The game module failed to compile", false);
		return;
	}

	// Only now, back on the main thread, is it safe to swap the module out.
	ReloadGameModule();
}

void EditorLayer::UnloadGameModule()
{
	// The simulation holds live instances of the module's types; wind it down first.
	if (mPlaying)
		StopPlay();

	// Nothing may outlive the module holding a component of its own: an entity kept alive by a stray
	// reference would run its components' destructors after the code was gone.
	SetSelection(nullptr);
	mRenamingEntity = nullptr;
	mEntityToDelete = nullptr;
	mReparentChild = nullptr;
	mReparentTarget = nullptr;
	mEntityLookup.clear();
	mScene->Clear();

	Lion::UnloadGameModule(mGameModule);
}

void EditorLayer::ReloadGameModule()
{
	// Every component the module defines is about to lose the code behind its vtable, so the scene
	// makes a round trip through its serialized form: those components are dropped now and recreated
	// by name from the freshly loaded module. Undo/redo and the clipboard are plain text already.
	const std::string scene = SceneSerializer::SerializeToString(mScene);
	const int32 selected = SelectedEntityIndex();

	UnloadGameModule();

	const bool loaded = LoadGameModule();

	// Restore the scene either way: without the module its components are simply skipped, which beats
	// throwing the scene away because a rebuild was broken.
	SceneSerializer::DeserializeFromString(mScene, scene);
	SelectEntityByIndex(selected);

	if (loaded)
	{
		Log::Console(LogLevel::Success, "[Editor] Reloaded the game module.");
		DismissBusyToasts();
		PushToast("Reloaded the game module", false);
	}
}

bool EditorLayer::GenerateComponent(const std::string& name, const std::string& folder)
{
	const std::filesystem::path directory = ComponentDirectory(folder);

	if (directory.empty())
	{
		Log::Console(LogLevel::Error, "[Editor] Could not locate the game's assets; is the project checked out?");
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

	std::ofstream header(headerPath);
	std::ofstream source(sourcePath);

	if (!header.is_open() || !source.is_open())
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not write '{}'.", directory.generic_string()));
		return false;
	}

	// Always a Component, never another component: an entity is what things are made of, and a component
	// is one trait of it. A trait that needs another one asks its owner for it — GetOwner() is right
	// there — and that reads better than a class that is two things at once.
	header
		<< "#pragma once\n\n"
		<< "#include <Lion/Lion.h>\n"
		<< "\n"
		<< "// Component defined by the game. Being compiled into the game module is all it takes for the\n"
		<< "// editor to list it under Add Component: loading the module registers it with the engine.\n"
		<< "//\n"
		<< "// Override the lifecycle hooks to give it behaviour, and name a field in Reflect to have it\n"
		<< "// appear in the Inspector and be saved with the scene — describing it once is describing it.\n"
		<< "// GetOwner() reaches the entity it is attached to, and through it every other component on it.\n"
		<< "class " << name << " : public Lion::Component\n"
		<< "{\n"
		<< "public:\n"
		<< "\tvoid OnAwake() override;\n"
		<< "\tvoid OnUpdate() override;\n\n"
		<< "\tvoid Reflect(Lion::Reflector& reflector) override;\n\n"
		<< "private:\n"
		<< "\tLion::float32 mSpeed = 1.0f;\n"
		<< "};\n";

	source
		<< "#include \"" << name << ".h\"\n\n"
		<< "#include <Lion/Logic/ComponentRegistry.h>\n"
		<< "#include <Lion/Logic/Reflector.h>\n\n"
		<< "using namespace Lion;\n\n"
		<< "void " << name << "::OnAwake()\n"
		<< "{\n"
		<< "}\n\n"
		<< "void " << name << "::OnUpdate()\n"
		<< "{\n"
		<< "}\n\n"
		<< "// The fields the editor shows and the scene file keeps. One list, both jobs.\n"
		<< "void " << name << "::Reflect(Reflector& reflector)\n"
		<< "{\n"
		<< "\treflector.Field(\"Speed\", mSpeed);\n"
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

	ImGui::TextUnformatted("Class name");

	if (ImGui::IsWindowAppearing())
		ImGui::SetKeyboardFocusHere();

	ImGui::SetNextItemWidth(320.0f);
	const bool submitted = ImGui::InputText("##name", mNewComponentName, IM_ARRAYSIZE(mNewComponentName),
		ImGuiInputTextFlags_EnterReturnsTrue);

	// Where it lands, as a path under the game's assets — the same paths the Project panel browses. It
	// keeps whatever was typed last: a component rarely arrives alone. The folder can be typed or picked;
	// what the picker gives back is an absolute path, and what is stored is that path made relative to the
	// assets, because a component lives inside the game's tree and an absolute one only names this machine.
	ImGui::Spacing();
	ImGui::TextUnformatted("Folder");

	const ImGuiStyle& style = ImGui::GetStyle();
	const float32 browseWidth = ImGui::CalcTextSize(ICON_MDI_FOLDER_OPEN).x + style.FramePadding.x * 2.0f;

	ImGui::SetNextItemWidth(320.0f - browseWidth - style.ItemInnerSpacing.x);
	ImGui::InputText("##folder", mNewComponentFolder, IM_ARRAYSIZE(mNewComponentFolder));

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

	ImGui::BeginDisabled(GameAssetsDirectory().empty());
	if (ImGui::Button(ICON_MDI_FOLDER_OPEN))
	{
		const std::filesystem::path assetsRoot = GameAssetsDirectory();
		const std::string picked = FileDialog::OpenFolder((assetsRoot / mNewComponentFolder).string());

		if (!picked.empty())
		{
			// Back to a path relative to the assets. A folder chosen outside them cannot hold a component,
			// so it is refused here rather than written and rejected on Create.
			std::error_code relativeError;
			const std::filesystem::path relative = std::filesystem::relative(picked, assetsRoot, relativeError);
			const std::string relativeText = relative.generic_string();

			if (relativeError || relativeText.empty() || relativeText.rfind("..", 0) == 0)
				Log::Console(LogLevel::Warning, "[Editor] A component has to live inside the game's assets.");
			else
			{
				const size_t copied = relativeText.copy(mNewComponentFolder, sizeof(mNewComponentFolder) - 1);
				mNewComponentFolder[copied] = '\0';
			}
		}
	}
	ImGui::EndDisabled();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Browse for a folder inside the game's assets");

	const std::string name = mNewComponentName;
	const std::filesystem::path assets = GameAssetsDirectory();
	const std::filesystem::path directory = ComponentDirectory(mNewComponentFolder);
	const bool valid = IsValidTypeName(name) && !directory.empty() && !ComponentRegistry::Contains(name);

	ImGui::Spacing();

	if (assets.empty())
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "The game's assets were not found.");
	else if (directory.empty())
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "The folder has to stay inside the game's assets.");
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
		GenerateComponent(name, mNewComponentFolder);
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndDisabled();
	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}

namespace
{
	// The recent scenes live beside the editor's other state, one path per line, newest first — the same
	// Data/ folder the layout and shortcuts use. The recent projects live there too, but through
	// Projects.h: the Project Manager window reads the same list.
	std::filesystem::path RecentScenesFile()
	{
		return EditorDataDirectory() / "recent-scenes.txt";
	}
}

void EditorLayer::LoadRecentProjects()
{
	mRecentProjects = Projects::LoadRecent();
}

void EditorLayer::RememberRecentProject(const std::filesystem::path& folder)
{
	Projects::Remember(folder);
	LoadRecentProjects();
}

void EditorLayer::LoadRecentScenes()
{
	mRecentScenes.clear();

	std::ifstream file(RecentScenesFile());
	std::string line;

	while (std::getline(file, line))
	{
		while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
			line.pop_back();

		std::error_code error;

		// A scene moved or deleted since it was last opened is dropped, not offered as a dead end.
		if (!line.empty() && std::filesystem::is_regular_file(line, error))
			mRecentScenes.push_back(line);
	}
}

void EditorLayer::SaveRecentScenes() const
{
	std::error_code error;
	std::filesystem::create_directories(EditorDataDirectory(), error);

	std::ofstream file(RecentScenesFile(), std::ios::trunc);

	for (const std::string& path : mRecentScenes)
		file << path << '\n';
}

void EditorLayer::RememberRecentScene(const std::string& path)
{
	const std::string normalized = std::filesystem::path(path).generic_string();

	mRecentScenes.erase(std::remove(mRecentScenes.begin(), mRecentScenes.end(), normalized), mRecentScenes.end());
	mRecentScenes.insert(mRecentScenes.begin(), normalized);

	constexpr size_t kMaxRecentScenes = 10;

	if (mRecentScenes.size() > kMaxRecentScenes)
		mRecentScenes.resize(kMaxRecentScenes);

	SaveRecentScenes();
}

void EditorLayer::BrowseForProject()
{
	const std::filesystem::path active = ActiveProjectDirectory();
	const std::string picked = FileDialog::OpenFolder(active.empty() ? std::string() : active.generic_string());

	if (!picked.empty())
		OpenProject(picked);
}

void EditorLayer::OpenProject(const std::filesystem::path& folder)
{
	if (!IsProjectFolder(folder))
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] '{}' is not a Lion project (no Assets folder).", folder.generic_string()));
		PushToast("That folder is not a Lion project", false);
		return;
	}

	RememberRecentProject(folder);

	// Opening the project already in hand is not a switch: nothing to rebuild, nothing to replace.
	if (ActiveProjectDirectory().lexically_normal() == folder.lexically_normal())
		return;

	SetActiveProjectDirectory(folder);

	// The Content Browser returns to the new project's root, and its listing is stale by definition.
	mProjectPath.clear();
	mProjectDirty = true;

	// The editor initialises on the project, the way an engine opens a game: what was open belonged to the
	// project being left, and so did its history. The built-in Sandbox comes up on its demo scene; any
	// other project on a fresh one.
	mScene->Clear();
	SetSelection(nullptr);
	mScenePath.clear();
	mUndoStack.clear();
	mRedoStack.clear();

	if (folder.lexically_normal() == Projects::DefaultProjectDirectory().lexically_normal())
		CreateDemoScene();

	const std::string name = ProjectDisplayName(folder);
	Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[Editor] Opened project '{}'.", name));
	PushToast("Opened " + name, false);

	// Build this project's module and hot-swap it in, so its own components are what the editor now lists —
	// the one module the editor holds follows whichever project is open. Assets and built-in components work
	// without it; a project's own C++ waits on this. The built-in builds from the engine's tree, any other
	// project from the SDK beside the editor; with neither around, the switch is quiet — the module already
	// on disk is the one that runs, and the explanation belongs to the explicit Compile.
	const bool switchedToBuiltIn =
		folder.lexically_normal() == Projects::DefaultProjectDirectory().lexically_normal();

	if (switchedToBuiltIn ? !Projects::EngineRootDirectory().empty() : ProjectBuild::Available())
		CompileGameModule();
}

bool EditorLayer::CreateProject(const std::string& name, const std::filesystem::path& location)
{
	// The scaffolding lives in Projects.h, shared with the Project Manager window; this adds what only the
	// editor has — the console, the toasts, and an editor to open the result in.
	std::string error;
	const std::filesystem::path folder = Projects::CreateOnDisk(name, location, error);

	if (folder.empty())
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Editor] Could not create the project: {}", error));
		PushToast("Could not create the project", false);
		return false;
	}

	Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[Editor] Created project '{}'.", name));
	OpenProject(folder);
	return true;
}

void EditorLayer::DrawProjectManagerPopup()
{
	if (mOpenProjectManagerPopup)
	{
		mOpenProjectManagerPopup = false;
		mNewProjectName[0] = '\0';
		mNewProjectLocation[0] = '\0';
		ImGui::OpenPopup("Project Manager");
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(540.0f, 0.0f), ImGuiCond_Appearing);

	if (!ImGui::BeginPopupModal("Project Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	const std::filesystem::path active = ActiveProjectDirectory();

	// --- The project the editor has open.
	ImGui::TextDisabled("Current project");
	ImGui::PushFont(EditorGui::GetBoldFont());
	ImGui::TextUnformatted(active.empty() ? "No project open" : ProjectDisplayName(active).c_str());
	ImGui::PopFont();

	if (!active.empty())
		ImGui::TextDisabled("%s", active.generic_string().c_str());

	ImGui::Separator();

	// --- The ones opened before, double-click to switch.
	ImGui::TextDisabled("Recent");

	std::string openRequest;   // Applied after the loop, so the list is not switched out mid-draw.

	if (mRecentProjects.empty())
		ImGui::TextDisabled("  Nothing yet — open or create a project below.");

	for (const std::string& path : mRecentProjects)
	{
		ImGui::PushID(path.c_str());

		const bool isActive = (std::filesystem::path(path) == active);
		const std::string label = std::string(ICON_MDI_FOLDER "  ") + ProjectDisplayName(path);

		// DontClosePopups: a Selectable dismisses its host popup on any click by default, which would take
		// the first click of the double-click and the modal with it. Closing is the open request's job.
		if (ImGui::Selectable(label.c_str(), isActive,
			ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_DontClosePopups)
			&& ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			openRequest = path;

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s\nDouble-click to open", path.c_str());

		ImGui::PopID();
	}

	ImGui::Separator();

	// --- A fresh project, scaffolded from the template.
	ImGui::TextDisabled("New project");

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##project_name", "Project name", mNewProjectName, sizeof(mNewProjectName));

	const ImGuiStyle& style = ImGui::GetStyle();
	const float32 browseWidth = ImGui::CalcTextSize(ICON_MDI_FOLDER_OPEN).x + style.FramePadding.x * 2.0f;

	ImGui::SetNextItemWidth(-browseWidth - style.ItemInnerSpacing.x);
	ImGui::InputTextWithHint("##project_location", "Location", mNewProjectLocation, sizeof(mNewProjectLocation));

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

	if (ImGui::Button(ICON_MDI_FOLDER_OPEN "##pick_location"))
	{
		const std::string picked = FileDialog::OpenFolder(mNewProjectLocation);

		if (!picked.empty())
		{
			const size_t copied = picked.copy(mNewProjectLocation, sizeof(mNewProjectLocation) - 1);
			mNewProjectLocation[copied] = '\0';
		}
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Choose where the project folder is created");

	const std::string newName = mNewProjectName;
	const bool canCreate = IsValidLayoutName(newName) && mNewProjectLocation[0] != '\0';

	if (newName[0] != '\0' && !IsValidLayoutName(newName))
		ImGui::TextColored(LogLevelColor(LogLevel::Error), "Letters, digits, spaces, - and _ only.");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// --- Actions.
	if (ImGui::Button(ICON_MDI_FOLDER_OPEN "  Open Folder...", ImVec2(150.0f, 0.0f)))
	{
		const std::string picked = FileDialog::OpenFolder(active.empty() ? std::string() : active.generic_string());

		if (!picked.empty())
			openRequest = picked;
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(!canCreate);

	if (ImGui::Button("Create Project", ImVec2(150.0f, 0.0f)))
	{
		if (CreateProject(newName, mNewProjectLocation))
			ImGui::CloseCurrentPopup();
	}

	ImGui::EndDisabled();
	ImGui::SameLine();

	if (ImGui::Button("Close", ImVec2(90.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		ImGui::CloseCurrentPopup();

	if (!openRequest.empty())
	{
		OpenProject(openRequest);
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}
