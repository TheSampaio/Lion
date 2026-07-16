#pragma once

#include <string>
#include <vector>

#include <Lion/Lion.h>

#include "../Projects.h"

// The window that greets before the editor: the projects this machine has, one double-click from being
// worked on — the way Godot, Unity and Unreal open. It never becomes the editor; picking a project starts
// the editor on it as its own process and bows out, so each editor run belongs to exactly one game.
//
// It wears the same custom caption the editor does — the mark, the centred name, the coloured buttons —
// but it is a picker, not a workspace: a fixed size, no maximise, just minimise and close.
class ProjectManagerLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnCreate() override;
	void OnRender() override;
	void OnDetach() override;

private:
	// How the list is ordered. Favourites sort ahead of everything whichever is chosen.
	enum class Sort { LastEdited, Name, Path };

	void DrawManager();
	void DrawCaption(Lion::float32 width);
	void DrawBanner(Lion::float32 width);
	void DrawToolbar();
	void DrawProjectList();
	void DrawActionsPanel();
	void DrawPopups();

	// The list, re-read from disk after anything that changes it.
	void Refresh();

	// The selected entry, or null when the selection points at nothing anymore.
	const Projects::Entry* Selected() const;

	// Starts the editor on the project and closes the manager.
	void OpenInEditor(const std::string& project);

	std::vector<Projects::Entry> mEntries;
	std::string mSelected;   // The selected project's path; what the action buttons act on.
	char mFilter[64] = {};
	Sort mSort = Sort::LastEdited;

	// The popups: creating, renaming, removing — each opened by a flag so the button click and the modal
	// live in different scopes, and each with the fields it edits.
	bool mOpenCreatePopup = false;
	char mName[64] = {};
	char mLocation[512] = {};
	std::string mCreateError;

	bool mOpenRenamePopup = false;
	char mRenameBuffer[64] = {};
	std::string mRenameError;

	bool mOpenRemovePopup = false;

	Lion::Reference<Lion::Texture> mLogo;
	Lion::Reference<Lion::Texture> mBanner;
};
