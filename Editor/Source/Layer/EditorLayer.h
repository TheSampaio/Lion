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
	void OnRender() override;
	void OnDetach() override;

private:
	bool mShowDemo = false;
	bool mLayoutInitialized = false;
	ImGuizmo::OPERATION mGizmoOperation = ImGuizmo::TRANSLATE;

	Lion::Reference<Lion::CameraOrthographic> mCamera;
	Lion::Reference<Lion::Scene> mScene;
	Lion::Reference<Lion::Framebuffer> mFramebuffer;
	Lion::Reference<Lion::Entity> mSelectedEntity;
	glm::vec2 mViewportSize{ 0.0f, 0.0f };

	void CreateDemoScene();
	void RenderScene();
	void DrawUI();
	void DrawMenuBar();
	void DrawViewport();
	void DrawHierarchy();
	void DrawProperties();
	void BuildDefaultLayout(unsigned int dockspaceId);
};
