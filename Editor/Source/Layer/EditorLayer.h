#pragma once

#include <Lion/Lion.h>

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
		Count
	};

	struct Keybind
	{
		ImGuiKey key = ImGuiKey_None;
		bool ctrl = false;
		bool shift = false;
		bool alt = false;
	};

	Keybind mBinds[static_cast<int>(ShortcutAction::Count)];
	int mRebindingIndex = -1;  // Index of the action currently capturing a new key (-1 = none).

	bool mShowDemo = false;
	bool mShowShortcuts = false;
	bool mLayoutInitialized = false;
	bool mConsoleAutoScroll = true;
	bool mConsoleShowErrors = true;    // Console severity filters (Error/Fatal, Warning, everything else).
	bool mConsoleShowWarnings = true;
	bool mConsoleShowInfo = true;
	bool mPlaying = false;
	bool mPaused = false;        // In play mode but the simulation is halted.
	bool mShowColliders = true;  // Draw collider hitbox outlines over the viewport.
	bool mRenameFocus = false;  // Request keyboard focus on the inline rename field for one frame.
	ImGuizmo::OPERATION mGizmoOperation = ImGuizmo::TRANSLATE;

	Lion::Reference<Lion::CameraOrthographic> mCamera;
	Lion::Reference<Lion::Scene> mScene;
	Lion::Reference<Lion::Framebuffer> mFramebuffer;
	Lion::Reference<Lion::Entity> mSelectedEntity;
	Lion::Reference<Lion::Entity> mRenamingEntity;  // Entity whose name is being edited inline (F2 / context menu).
	glm::vec2 mViewportSize{ 0.0f, 0.0f };

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
	void DrawViewport();
	void DrawColliderOverlays(const ImVec2& imageMin, const ImVec2& imageSize);
	void DrawHierarchy();
	void DrawEntityNode(const Lion::Reference<Lion::Entity>& entity);
	void DrawProperties();
	void DrawConsole();
	void DrawProject();
	void DrawShortcuts();
	void BuildDefaultLayout(unsigned int dockspaceId);

	// Draws a collapsing header for a component with a right-aligned "X" remove button and
	// drag-to-reorder support. Returns whether the body is open; sets removeRequested when the X is
	// pressed, and sets dragFrom/dragTo when a header is dropped onto this one.
	bool DrawComponentHeader(const char* label, int index, bool& removeRequested, int& dragFrom, int& dragTo);

	// Draws an X/Y/Z vector editor with red/green/blue axis buttons (Unity/Godot-style). Clicking an
	// axis button resets that component to resetValue. Returns whether any value changed.
	bool DrawVec3Control(const char* label, float values[3], float speed, float resetValue);

	// Undo/redo helpers. RecordSnapshot is for discrete actions (add/delete); BeginEdit/CommitEdit
	// group a continuous edit (a gizmo or slider drag) into a single undo step.
	void RecordSnapshot();
	void BeginEdit();
	void CommitEdit();
	void Undo();
	void Redo();
	void HandleShortcuts();

	// Entity clipboard: copy the selection, paste a new entity from the clipboard, or do both at
	// once (duplicate). The new entity is appended to the scene and becomes the selection.
	void CopyEntity();
	void PasteEntity();
	void DuplicateEntity();

	// Creates an organizational folder entity (no components, identity transform).
	void CreateFolder();

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
