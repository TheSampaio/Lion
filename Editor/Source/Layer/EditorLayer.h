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
	bool mShowDemo = false;
	bool mLayoutInitialized = false;
	bool mConsoleAutoScroll = true;
	bool mPlaying = false;
	ImGuizmo::OPERATION mGizmoOperation = ImGuizmo::TRANSLATE;

	Lion::Reference<Lion::CameraOrthographic> mCamera;
	Lion::Reference<Lion::Scene> mScene;
	Lion::Reference<Lion::Framebuffer> mFramebuffer;
	Lion::Reference<Lion::Entity> mSelectedEntity;
	glm::vec2 mViewportSize{ 0.0f, 0.0f };

	// Undo/redo history of full-scene JSON snapshots. mPendingSnapshot holds the pre-edit state
	// captured at the start of a continuous edit (gizmo/drag), committed once the edit ends.
	std::vector<std::string> mUndoStack;
	std::vector<std::string> mRedoStack;
	std::string mPendingSnapshot;
	bool mHasPending = false;

	// Snapshot of the edited scene captured when entering Play mode, restored on Stop.
	std::string mPlaySnapshot;

	static constexpr size_t kMaxUndo = 100;

	void CreateDemoScene();
	void RenderScene();
	void DrawUI();
	void DrawMenuBar();
	void DrawViewport();
	void DrawHierarchy();
	void DrawProperties();
	void DrawConsole();
	void BuildDefaultLayout(unsigned int dockspaceId);

	// Undo/redo helpers. RecordSnapshot is for discrete actions (add/delete); BeginEdit/CommitEdit
	// group a continuous edit (a gizmo or slider drag) into a single undo step.
	void RecordSnapshot();
	void BeginEdit();
	void CommitEdit();
	void Undo();
	void Redo();
	void HandleShortcuts();

	// Play mode: StartPlay snapshots the edited scene and re-awakes it (creating physics bodies) so
	// the simulation runs; StopPlay restores the snapshot, returning to the edited state.
	void StartPlay();
	void StopPlay();
};
