#include "PauseMenu.h"
#include "GameRules.h"
#include "GameSettings.h"
#include "ScreenTransition.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void PauseMenu::Initialize()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> overlay = scene->FindEntity("Pause Overlay");
	const Reference<Entity> resume = scene->FindEntity("Resume Button");
	const Reference<Entity> mainMenu = scene->FindEntity("Pause Main Menu Button");
	const Reference<Entity> controllerPrompts = scene->FindEntity("Pause Controller Prompts");
	const Reference<Entity> keyboardPrompts = scene->FindEntity("Pause Keyboard Prompts");
	mOverlay = overlay.get();
	mControllerPrompts = controllerPrompts.get();
	mKeyboardPrompts = keyboardPrompts.get();
	mResumeButton = resume ? resume->GetComponent<Button>() : nullptr;
	mMainMenuButton = mainMenu ? mainMenu->GetComponent<Button>() : nullptr;
	const Reference<Entity> title = scene->FindEntity("Pause Title");
	const Reference<Entity> subtitle = scene->FindEntity("Pause Subtitle");
	if (TextRenderer* text = title ? title->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::Paused));
	if (TextRenderer* text = subtitle ? subtitle->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::CircuitSuspended));
	if (TextRenderer* text = resume ? resume->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::Resume));
	if (TextRenderer* text = mainMenu ? mainMenu->GetComponent<TextRenderer>() : nullptr)
		text->SetText(GameSettings::Text(GameText::MainMenu));
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
		if (Input::GetActionTap("player_pause"))
		{
			Show(true);
			SceneManager::SetPaused(true);
		}
		return;
	}

	UpdateInputPrompts();

	if (mResumeButton && mResumeButton->IsHovered() && mSelection != 0)
	{
		mSelection = 0;
		RefreshSelection();
	}
	else if (mMainMenuButton && mMainMenuButton->IsHovered() && mSelection != 1)
	{
		mSelection = 1;
		RefreshSelection();
	}

	if ((mResumeButton && mResumeButton->WasClicked()) || Input::GetActionTap("menu_back")
		|| Input::GetActionTap("player_pause"))
	{
		Show(false);
		SceneManager::SetPaused(false);
	}
	else if (mMainMenuButton && mMainMenuButton->WasClicked())
	{
		SceneManager::SetPaused(false);
		GameRules::AbandonSession();
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
	}
	else if (Input::GetActionTap("menu_up") || Input::GetActionTap("menu_down"))
	{
		mSelection = 1 - mSelection;
		RefreshSelection();
	}
	else if (Input::GetActionTap("menu_confirm"))
		ActivateSelection();
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

	if (visible)
	{
		mSelection = 0;
		RefreshSelection();
		UpdateInputPrompts();
	}
}

void PauseMenu::RefreshSelection()
{
	if (mResumeButton)
		mResumeButton->SetSelected(mSelection == 0);
	if (mMainMenuButton)
		mMainMenuButton->SetSelected(mSelection == 1);
}

void PauseMenu::UpdateInputPrompts()
{
	const bool hints = SceneManager::IsPaused() && GameSettings::HasControlHints();
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

void PauseMenu::ActivateSelection()
{
	if (mSelection == 0)
	{
		Show(false);
		SceneManager::SetPaused(false);
	}
	else
	{
		SceneManager::SetPaused(false);
		GameRules::AbandonSession();
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
	}
}

LION_REGISTER_COMPONENT(PauseMenu)
