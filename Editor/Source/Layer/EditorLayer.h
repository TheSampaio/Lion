#pragma once

#include <future>
#include <optional>

#include <Lion/Lion.h>
#include <Lion/Core/DynamicLibrary.h>

#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

// Root editor layer: sets up the window, renders a scene into a framebuffer and draws the
// docked editor UI (menu bar + panels), displaying the framebuffer inside the Viewport panel.
class EditorLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnCreate() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDetach() override;

private:
	// Rebindable editor shortcuts. Each action holds a key plus modifier flags; the Shortcuts panel
	// lets the user change them, and the handlers query them via IsShortcutPressed.
	// New actions are appended so indices already saved in shortcuts.cfg stay valid.
	enum class ShortcutAction
	{
		Undo, Redo, Play, Stop, ToggleShortcuts,
		GizmoMove, GizmoRotate, GizmoScale, RenameEntity, DeleteEntity,
		Pause, ToggleColliders,
		CopyEntity, PasteEntity, DuplicateEntity,
		ToolSelect,
		ReloadModule,
		StepFrame, CompileModule,
		ToggleHierarchy, ToggleProject, ToggleConsole, ToggleStatistics, ToggleProperties,
		Count
	};

	// Viewport tools, mirroring the Q/W/E/R row in the top-left corner.
	enum class Tool { Select, Move, Rotate, Scale };

	// A dock-layout change requested from the menu, deferred to the top of the next frame.
	enum class LayoutRequest { None, Reset, Load };

	struct Keybind
	{
		ImGuiKey key = ImGuiKey_None;
		bool ctrl = false;
		bool shift = false;
		bool alt = false;
	};

	Keybind mBinds[static_cast<int>(ShortcutAction::Count)];
	int mRebindingIndex = -1;  // Index of the action currently capturing a new key (-1 = none).

	bool mShowShortcuts = false;
	bool mLayoutInitialized = false;
	bool mFocusViewport = false;   // Set at boot, consumed a frame later: focus cannot be claimed while the panels are still appearing.

	// Panels the View menu can hide. The Viewport is not among them: hiding the thing being edited is
	// not a view option. A panel closed with its own X flips the same flag, so the menu always agrees
	// with what is on screen.
	bool mShowHierarchy = true;
	bool mShowProject = true;
	bool mShowConsole = true;
	bool mShowStatistics = true;
	bool mShowProperties = true;

	// The panels, in the order the shortcuts number them — which traces the default layout: down the
	// left column, along the bottom, then down the right one. One table, so the View menu and the
	// shortcut handler cannot disagree about what exists.
	struct Panel
	{
		const char* name;
		bool EditorLayer::* visible;
		ShortcutAction shortcut;
	};

	static const Panel kPanels[5];

	// Dock layouts, Unity-style: the built-in default plus any the user saved under "Layouts/".
	// Switching one in tears down every dock node, so the menu only records the request here and
	// ApplyPendingLayout consumes it before the next frame submits a window.
	LayoutRequest mLayoutRequest = LayoutRequest::None;
	std::string mLayoutToLoad;
	unsigned int mDockspaceId = 0;   // Captured each frame, so the deferred rebuild can address it.
	bool mOpenSaveLayoutPopup = false;
	char mLayoutName[64] = {};

	// "New C++ Component": scaffolds a component class into the game's source tree. The Add Component
	// popup only records the request, since it closes on click and takes the modal's ID scope with it.
	bool mOpenNewComponentPopup = false;
	char mNewComponentName[64] = {};
	char mNewComponentFolder[64] = "Scripts";   // Where it lands, under the game's assets.
	bool mConsoleAutoScroll = true;
	bool mConsoleShowErrors = true;    // Console severity filters (Error/Fatal, Warning, everything else).
	bool mConsoleShowWarnings = true;
	bool mConsoleShowInfo = true;

	// Console rendering: only the entries passing the filters are indexed here, and a list clipper
	// draws just the visible slice, so a full history costs the same as a screenful.
	std::vector<int> mConsoleVisible;
	int mConsoleSelected = -1;
	size_t mConsoleLastTotal = 0;   // Total lines logged as of the last drawn frame; drives the tail follow.
	bool mPlaying = false;
	bool mPaused = false;        // In play mode but the simulation is halted.
	bool mStepFrame = false;     // Advance the paused simulation by exactly one frame, then halt again.
	bool mShowColliders = false;  // Collider outlines are a debug view, off until enabled in Settings.
	bool mRenameFocus = false;   // Request keyboard focus on the inline rename field for one frame.
	bool mScaleUniform = false;  // The Transform's scale padlock: the three axes move together.
	Tool mTool = Tool::Move;

	// The game module, loaded at startup so its components register with the engine and appear in the
	// Add Component list. Held for the editor's lifetime, so the game's code stays mapped.
	Lion::DynamicLibrary mGameModule;

	Lion::Reference<Lion::CameraOrthographic> mCamera;
	Lion::Reference<Lion::Scene> mScene;
	Lion::Reference<Lion::Framebuffer> mFramebuffer;
	// The selection, and the one entity in it that everything single-target reads: the last one clicked.
	// The Inspector shows that one and writes to all of them, which is what "editing what they have in
	// common" means — the others are only touched where they have the same thing to touch.
	Lion::Reference<Lion::Entity> mSelectedEntity;
	std::vector<Lion::Reference<Lion::Entity>> mSelection;

	Lion::Reference<Lion::Entity> mRenamingEntity;  // Entity whose name is being edited inline (F2 / context menu).
	char mHierarchyFilter[64] = {};                 // The Hierarchy's search box: a name, or part of one.
	std::string mScenePath;                         // The scene on disk, empty until it has been saved once.
	glm::vec2 mViewportSize{ 0.0f, 0.0f };
	Lion::Vector mViewportMenuPosition;   // Where the mouse was when the viewport's context menu opened.

	// Undo/redo history of full-scene JSON snapshots. mPendingSnapshot holds the pre-edit state
	// captured at the start of a continuous edit (gizmo/drag), committed once the edit ends.
	std::vector<std::string> mUndoStack;
	std::vector<std::string> mRedoStack;
	std::string mPendingSnapshot;
	bool mHasPending = false;

	// Snapshot of the edited scene captured when entering Play mode, restored on Stop.
	std::string mPlaySnapshot;

	// Serialized entity held by Ctrl+C, pasted by Ctrl+V.
	std::string mEntityClipboard;

	// Project panel: the folder currently being browsed, relative to the resource root.
	std::string mProjectPath;

	// Hierarchy tree state, applied after the tree is drawn (never mutate it mid-iteration).
	std::unordered_map<Lion::Entity*, Lion::Reference<Lion::Entity>> mEntityLookup;
	Lion::Reference<Lion::Entity> mEntityToDelete;
	Lion::Entity* mReparentChild = nullptr;
	Lion::Entity* mReparentTarget = nullptr;   // Null target with mReparentChild set means "to root".
	bool mReparentRequested = false;

	static constexpr size_t kMaxUndo = 100;

	void CreateDemoScene();
	void RenderScene();
	void DrawUI();
	void DrawMenuBar();
	void DrawStatusBar();   // The bar along the bottom: what is open, and which engine has it open.
	void DrawViewport();
	void DrawColliderOverlays(const ImVec2& imageMin, const ImVec2& imageSize);
	void DrawHierarchy();
	void DrawEntityNode(const Lion::Reference<Lion::Entity>& entity);
	void DrawProperties();
	void DrawConsole();
	void DrawProject();
	void DrawShortcuts();
	void BuildDefaultLayout(unsigned int dockspaceId);

	// Dock layout management (Window > Layouts). A layout is just an imgui.ini saved under
	// "Layouts/<name>.ini", so loading one restores every panel's dock node, position and size.
	void ApplyPendingLayout();                        // Consumes a pending reset/load, before any window is drawn.
	void DrawLayoutMenu();                            // The "Layouts" submenu: default, saved ones, save, delete.
	void DrawLayoutPopups();                          // The "Save Layout" modal, drawn at the root.
	void SaveLayout(const std::string& name) const;
	std::vector<std::string> SavedLayouts() const;    // Names of the layouts on disk, sorted.

	static std::string LayoutPath(const std::string& name);
	static bool IsValidLayoutName(const std::string& name);  // Rejects names that would escape the folder.

	// "New C++ Component" (Unreal-style): name the type, and the editor writes the
	// .h/.cpp into the game's source tree. The new class is compiled into the game module on the next
	// build, which is when it starts showing up in Add Component.
	void DrawNewComponentPopup();
	bool GenerateComponent(const std::string& name, const std::string& folder);

	// Game module lifecycle. LoadGameModule loads a copy of Game.dll, leaving the original writable so
	// it can be rebuilt while the editor runs; ReloadGameModule swaps in that rebuild, taking the
	// scene out and back through its serialized form so no object outlives the code behind it.
	bool LoadGameModule();
	void ReloadGameModule();

	// Drops the module and everything that is code inside it — the registries' factories, and the
	// components whose vtables live there. A reload does this before loading the rebuilt module; the
	// editor does it before closing, because the process would otherwise unmap the library first.
	void UnloadGameModule();

	// Advances a paused simulation by a single frame (entering the paused state if it was running).
	void StepOneFrame();

	// Rebuilds the game module and reloads it, so a code change lands without leaving the editor. The
	// build runs on a worker thread; PollGameBuild picks up the result on the main thread, which is
	// where the reload has to happen. Nothing else may touch the module while a build is in flight.
	void CompileGameModule();
	void PollGameBuild();

	struct GameBuild
	{
		int exitCode = 0;
		std::string output;
	};

	std::future<GameBuild> mGameBuild;
	bool mBuilding = false;

	// Draws a collapsing header for a component with a right-aligned "X" remove button and
	// drag-to-reorder support. Returns whether the body is open; sets removeRequested when the X is
	// pressed, and sets dragFrom/dragTo when a header is dropped onto this one.
	bool DrawComponentHeader(const char* label, int index, bool& removeRequested, int& dragFrom, int& dragTo);

	// Draws a scalar property row: the label, the field, and the revert arrow that puts the default
	// back — which only appears while the value is not it. Pass no default for a field that has none
	// worth reverting to (a collider's extents), and the row keeps the arrow's slot without one.
	// Returns whether the value changed.
	bool DrawFloatProperty(const Lion::char8* label, Lion::float32& value, Lion::float32 speed,
		Lion::float32 minimum, Lion::float32 maximum, std::optional<Lion::float32> defaultValue);

	// Draws an X/Y/Z vector editor with red/green/blue axis buttons (Unity/Godot-style). Clicking an
	// axis button resets that component to resetValue, and the revert arrow at the end of the row resets
	// all three — it only appears while they are not all resetValue, which is the field's default.
	//
	// Pass 'uniform' for a row that can be locked (the Scale): while it is set, editing one axis moves
	// the other two by the same proportion. Returns whether any value changed.
	bool DrawVec3Control(const char* label, float values[3], float speed, float resetValue, bool* uniform = nullptr);

	// Undo/redo helpers. RecordSnapshot is for discrete actions (add/delete); BeginEdit/CommitEdit
	// group a continuous edit (a gizmo or slider drag) into a single undo step.
	void RecordSnapshot();
	void BeginEdit();
	void CommitEdit();
	void Undo();
	void Redo();
	void HandleShortcuts();

	// Selection. One entity is the primary — the last one clicked — and the rest ride along with it.
	// Every assignment goes through these, so the two can never disagree about what is selected.
	void SetSelection(const Lion::Reference<Lion::Entity>& entity);      // Replaces the selection (or clears it, with null).
	void AddToSelection(const Lion::Reference<Lion::Entity>& entity);    // Ctrl+click: adds, or removes if already in.
	void SelectRangeTo(const Lion::Reference<Lion::Entity>& entity);     // Shift+click: everything between the primary and this one.
	bool IsSelected(const Lion::Entity* entity) const;

	// Commits an inline rename. One entity takes the name as typed; a whole selection takes it numbered,
	// which is what Windows does when several things are given one name.
	void RenameSelection(const std::string& name, const Lion::Reference<Lion::Entity>& renamed);

	// Runs 'apply' over every selected entity that carries a T, so an edit made on the one being shown
	// lands on the ones that have the same thing to change.
	template<typename T, typename Apply>
	void ApplyToSelection(Apply apply)
	{
		for (const auto& entity : mSelection)
			if (T* component = entity->GetComponent<T>())
				apply(component);
	}

	// Entity clipboard: copy the selection, paste a new entity from the clipboard, or do both at
	// once (duplicate). The new entity is appended to the scene and becomes the selection.
	void CopyEntity();
	void PasteEntity();
	void DuplicateEntity();

	// Creates an entity: under 'parent' when there is one, at 'position' when there is one (which is how
	// the viewport creates one where the mouse is). It becomes the selection, ready to be named.
	Lion::Reference<Lion::Entity> CreateEntity(Lion::Entity* parent = nullptr, const Lion::Vector* position = nullptr);

	// Creates an organizational folder entity (no components, identity transform).
	void CreateFolder();

	// The entity commands shared by the Hierarchy's context menu and the viewport's: they are the same
	// menu, so they are the same code — one of them showing up in two places is not two menus.
	void DrawEntityMenuItems(const Lion::Reference<Lion::Entity>& target, const Lion::Vector* position);

	// Shortcut/keybinding helpers.
	void InitShortcuts();                                  // Sets defaults, then loads overrides from disk.
	void ResetShortcutsToDefault();                        // Assigns the built-in default bindings.
	void LoadShortcuts();                                  // Overrides binds from the on-disk config, if present.
	void SaveShortcuts() const;                            // Persists the current binds to disk.
	bool IsShortcutPressed(ShortcutAction action) const;   // True when the bound combo was pressed this frame.
	std::string KeybindToString(const Keybind& bind) const;

	// Play mode: StartPlay snapshots the edited scene and re-awakes it (creating physics bodies) so
	// the simulation runs; StopPlay restores the snapshot, returning to the edited state. Both keep
	// the current selection by index (the entity list round-trips in order).
	void StartPlay();
	void StopPlay();
	void TogglePause();

	// Play/Stop/Collider controls drawn as an overlay on top of the viewport image.
	void DrawViewportToolbar(const ImVec2& imageMin, const ImVec2& imageSize);

	// Selection is stored by index across a scene rebuild (serialize/deserialize round-trip).
	int SelectedEntityIndex() const;
	void SelectEntityByIndex(int index);
};
