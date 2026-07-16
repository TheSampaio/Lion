#pragma once

#include <string>
#include <vector>

#include <Lion/Lion.h>

// The window that greets before the editor: the projects this machine has, one double-click from being
// worked on — the way Godot, Unity and Unreal open. It never becomes the editor; picking a project starts
// the editor on it as its own process and bows out, so each editor run belongs to exactly one game.
class ProjectManagerLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnCreate() override;
	void OnRender() override;
	void OnDetach() override;

private:
	void DrawManager();
	void DrawProjectList();
	void DrawCreateSection();

	// Starts the editor on the project and closes the manager.
	void OpenInEditor(const std::string& project);

	std::vector<std::string> mProjects;
	char mFilter[64] = {};

	// The "New project" drawer: its fields, whether it is open, and what its last attempt had to say.
	bool mCreating = false;
	char mName[64] = {};
	char mLocation[512] = {};
	std::string mCreateError;

	Lion::Reference<Lion::Texture> mLogo;
};
