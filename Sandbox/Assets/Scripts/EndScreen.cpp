#include "EndScreen.h"
#include "GameRules.h"
#include "GameSettings.h"
#include "ScreenTransition.h"

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
	if (TextRenderer* text = playAgainEntity ? playAgainEntity->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::PlayAgain));
	if (TextRenderer* text = mainMenuEntity ? mainMenuEntity->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::MainMenu));
	RefreshSelection();
	UpdateControllerPrompts();

	if (TextRenderer* title = titleEntity ? titleEntity->GetComponent<TextRenderer>() : nullptr)
		title->SetText(GameSettings::Text(victory ? GameText::CircuitClear : GameText::SystemFailure));

	if (TextRenderer* score = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr)
		score->SetText(LION_FORMAT_TEXT("{}\n{:06}", GameSettings::Text(GameText::TotalScore), GameRules::GetScore()));

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
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
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
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
}

LION_REGISTER_COMPONENT(EndScreen)
