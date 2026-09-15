#include "EndScreen.h"
#include "GameAudio.h"
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
	const Reference<Entity> keyboardPrompts = scene->FindEntity("End Keyboard Prompts");
	const bool victory = SceneManager::GetActivePath().find("Victory") != std::string::npos;
	if (victory)
		GameAudio::PlaySfx("Sounds/victory.wav", 0.78f);
	mPlayAgainButton = playAgainEntity ? playAgainEntity->GetComponent<Button>() : nullptr;
	mMainMenuButton = mainMenuEntity ? mainMenuEntity->GetComponent<Button>() : nullptr;
	mControllerPrompts = controllerPrompts.get();
	mKeyboardPrompts = keyboardPrompts.get();
	if (TextRenderer* text = playAgainEntity ? playAgainEntity->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::PlayAgain));
	if (TextRenderer* text = mainMenuEntity ? mainMenuEntity->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::MainMenu));
	RefreshSelection();
	UpdateInputPrompts();

	if (TextRenderer* title = titleEntity ? titleEntity->GetComponent<TextRenderer>() : nullptr)
		title->SetText(GameSettings::Text(victory ? GameText::CircuitClear : GameText::GameOver));

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
		if (!Input::GetKeyPress(KeyCode::AnyKey) && !Input::GetMouseButtonPress(0)
			&& Input::GetActionStrength("menu_confirm") <= 0.0f
			&& Input::GetActionStrength("menu_back") <= 0.0f)
			mInputArmed = true;

		return;
	}

	UpdateInputPrompts();

	if (mPlayAgainButton && mPlayAgainButton->IsHovered() && mSelection != 0)
	{
		mSelection = 0;
		GameAudio::PlayUiHover();
		RefreshSelection();
	}
	else if (mMainMenuButton && mMainMenuButton->IsHovered() && mSelection != 1)
	{
		mSelection = 1;
		GameAudio::PlayUiHover();
		RefreshSelection();
	}

	if (mPlayAgainButton && mPlayAgainButton->WasClicked())
	{
		GameAudio::PlayUiSelect();
		GameRules::StartNewGame();
	}
	else if ((mMainMenuButton && mMainMenuButton->WasClicked()) || Input::GetActionTap("menu_back"))
	{
		GameAudio::PlayUiSelect();
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
	}
	else if (Input::GetActionTap("menu_up") || Input::GetActionTap("menu_down"))
	{
		mSelection = 1 - mSelection;
		GameAudio::PlayUiHover();
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

void EndScreen::UpdateInputPrompts()
{
	const bool hints = GameSettings::HasControlHints();
	const bool gamepad = hints && Input::GetLastInputMethod() == InputMethod::Gamepad;
	const bool keyboard = hints && !gamepad;

	if (mControllerPrompts && gamepad != mShowingGamepadPrompts)
	{
		mShowingGamepadPrompts = gamepad;
		mControllerPrompts->SetVisible(gamepad);
		mControllerPrompts->SetEnabled(gamepad);
	}

	if (mKeyboardPrompts && keyboard != mShowingKeyboardPrompts)
	{
		mShowingKeyboardPrompts = keyboard;
		mKeyboardPrompts->SetVisible(keyboard);
		mKeyboardPrompts->SetEnabled(keyboard);
	}
}

void EndScreen::ActivateSelection()
{
	GameAudio::PlayUiSelect();
	if (mSelection == 0)
		GameRules::StartNewGame();
	else
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
}

LION_REGISTER_COMPONENT(EndScreen)
