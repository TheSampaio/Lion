#pragma once

#include <Lion/Lion.h>

// Root editor layer: sets up the window and draws the docked editor UI (menu bar + panels).
class EditorLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnRenderUI() override;

private:
	bool mShowDemo = false;
	bool mLayoutInitialized = false;

	void DrawMenuBar();
	void BuildDefaultLayout(unsigned int dockspaceId);
};
