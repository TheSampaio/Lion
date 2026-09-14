#include "EndScreen.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void EndScreen::OnAwake()
{
	// Scene path-dependent presentation is initialized on the first update; SceneManager publishes the
	// destination path only after all destination entities have Awakened.
}

void EndScreen::InitializeForScene()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> titleEntity = scene->FindEntity("Result Title");
	const Reference<Entity> scoreEntity = scene->FindEntity("Result Score");
	const Reference<Entity> playAgainEntity = scene->FindEntity("Play Again Button");
	const Reference<Entity> mainMenuEntity = scene->FindEntity("Main Menu Button");
	const Reference<Entity> controllerPrompts = scene->FindEntity("End Controller Prompts");
	const bool victory = SceneManager::GetActivePath().find("Victory") != std::string::npos;
	mPlayAgainButton = playAgainEntity ? playAgainEntity->GetComponent<Button>() : nullptr;
	mMainMenuButton = mainMenuEntity ? mainMenuEntity->GetComponent<Button>() : nullptr;
	mControllerPrompts = controllerPrompts.get();
	RefreshSelection();
	UpdateControllerPrompts();

	if (TextRenderer* title = titleEntity ? titleEntity->GetComponent<TextRenderer>() : nullptr)
		title->SetText(victory ? "CIRCUIT CLEAR" : "SYSTEM FAILURE");

	if (TextRenderer* score = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr)
		score->SetText(LION_FORMAT_TEXT("TOTAL SCORE\n{:06}", GameRules::GetScore()));

	Window::SetBackgroundColor(victory ? 0.015f : 0.08f, victory ? 0.08f : 0.015f, 0.035f);
}

void EndScreen::OnUpdate()
{
	if (!mInitialized)
	{
		InitializeForScene();
		return;
	}

	if (!mInputArmed)
	{
		if (Input::GetKeyRelease(KeyCode::AnyKey))
			mInputArmed = true;

		return;
	}

	UpdateControllerPrompts();

	if (mPlayAgainButton && mPlayAgainButton->IsHovered() && mSelection != 0)
	{
		mSelection = 0;
		RefreshSelection();
	}
	else if (mMainMenuButton && mMainMenuButton->IsHovered() && mSelection != 1)
	{
		mSelection = 1;
		RefreshSelection();
	}

	if (mPlayAgainButton && mPlayAgainButton->WasClicked())
		GameRules::StartNewGame();
	else if ((mMainMenuButton && mMainMenuButton->WasClicked()) || Input::GetActionTap("menu_back"))
		SceneManager::LoadScene("Scenes/MainMenu.lnscene");
	else if (Input::GetActionTap("menu_up") || Input::GetActionTap("menu_down"))
	{
		mSelection = 1 - mSelection;
		RefreshSelection();
	}
	else if (Input::GetActionTap("menu_confirm"))
		ActivateSelection();
}

void EndScreen::RefreshSelection()
{
	if (mPlayAgainButton)
		mPlayAgainButton->SetSelected(mSelection == 0);
	if (mMainMenuButton)
		mMainMenuButton->SetSelected(mSelection == 1);
}

void EndScreen::UpdateControllerPrompts()
{
	const bool show = Input::GetLastInputMethod() == InputMethod::Gamepad;

	if (!mControllerPrompts || show == mShowingControllerPrompts)
		return;

	mShowingControllerPrompts = show;
	mControllerPrompts->SetVisible(show);
	mControllerPrompts->SetEnabled(show);
}

void EndScreen::ActivateSelection()
{
	if (mSelection == 0)
		GameRules::StartNewGame();
	else
		SceneManager::LoadScene("Scenes/MainMenu.lnscene");
}

LION_REGISTER_COMPONENT(EndScreen)
