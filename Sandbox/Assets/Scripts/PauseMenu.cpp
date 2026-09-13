#include "PauseMenu.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void PauseMenu::Initialize()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> overlay = scene->FindEntity("Pause Overlay");
	const Reference<Entity> resume = scene->FindEntity("Resume Button");
	const Reference<Entity> mainMenu = scene->FindEntity("Pause Main Menu Button");
	mOverlay = overlay.get();
	mResumeButton = resume ? resume->GetComponent<Button>() : nullptr;
	mMainMenuButton = mainMenu ? mainMenu->GetComponent<Button>() : nullptr;
	Show(false);
}

void PauseMenu::OnUpdate()
{
	if (!mInitialized)
	{
		Initialize();
		return;
	}

	if (!SceneManager::IsPaused())
	{
		if (Input::GetActionTap("menu_back"))
		{
			Show(true);
			SceneManager::SetPaused(true);
		}
		return;
	}

	if ((mResumeButton && mResumeButton->WasClicked()) || Input::GetActionTap("menu_back"))
	{
		Show(false);
		SceneManager::SetPaused(false);
	}
	else if (mMainMenuButton && mMainMenuButton->WasClicked())
	{
		SceneManager::SetPaused(false);
		SceneManager::LoadScene("Scenes/MainMenu.lnscene");
	}
}

void PauseMenu::OnDestroy()
{
	SceneManager::SetPaused(false);
}

void PauseMenu::Show(bool visible)
{
	if (!mOverlay)
		return;

	mOverlay->SetVisible(visible);
	mOverlay->SetEnabled(visible);
}

LION_REGISTER_COMPONENT(PauseMenu)
