#include "MainMenu.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void MainMenu::OnAwake()
{
	Window::SetBackgroundColor(0.015f, 0.02f, 0.045f);
}

void MainMenu::Initialize()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> prompt = scene->FindEntity("Menu Prompt");
	const Reference<Entity> options = scene->FindEntity("Menu Options");
	const Reference<Entity> detail = scene->FindEntity("Menu Detail");
	const Reference<Entity> soundButton = scene->FindEntity("Sound Button");
	const Reference<Entity> backButton = scene->FindEntity("Back Button");
	const Reference<Entity> creditsLogo = scene->FindEntity("Credits Logo");
	const Reference<Entity> keyboardControls = scene->FindEntity("Menu Controls");
	const Reference<Entity> controllerAttractPrompt = scene->FindEntity("Controller Attract Prompt");
	const Reference<Entity> controllerMenuPrompts = scene->FindEntity("Controller Menu Prompts");
	const Reference<Entity> controllerDetailPrompts = scene->FindEntity("Controller Detail Prompts");
	const Reference<Entity> controllerSettingsPrompt = scene->FindEntity("Controller Settings Prompt");
	mPrompt = prompt.get();
	mOptions = options.get();
	mDetail = detail.get();
	mSoundButtonEntity = soundButton.get();
	mCreditsLogo = creditsLogo.get();
	mKeyboardControls = keyboardControls.get();
	mControllerAttractPrompt = controllerAttractPrompt.get();
	mControllerMenuPrompts = controllerMenuPrompts.get();
	mControllerDetailPrompts = controllerDetailPrompts.get();
	mControllerSettingsPrompt = controllerSettingsPrompt.get();
	mDetailText = mDetail ? mDetail->GetComponent<TextRenderer>() : nullptr;
	mSoundButtonText = mSoundButtonEntity ? mSoundButtonEntity->GetComponent<TextRenderer>() : nullptr;
	mSoundButton = mSoundButtonEntity ? mSoundButtonEntity->GetComponent<Button>() : nullptr;
	mBackButton = backButton ? backButton->GetComponent<Button>() : nullptr;

	static const char8* buttonNames[] = { "Play Button", "Credits Button", "Settings Button", "Quit Button" };
	for (int32 index = 0; index < static_cast<int32>(mMenuButtons.size()); ++index)
	{
		const Reference<Entity> button = scene->FindEntity(buttonNames[index]);
		mMenuButtons[index] = button ? button->GetComponent<Button>() : nullptr;
	}

	mSoundEnabled = Audio::GetBusVolume(AudioBus::Master) > 0.0f;
	ShowState(State::Attract);
}

void MainMenu::OnUpdate()
{
	// Assembly roots Awake before their authored children are added to the scene. Resolve the complete UI
	// on the first update, when every button is available.
	if (!mInitialized)
	{
		Initialize();
		return;
	}

	// Play is F5 in the editor. Requiring one completely released frame prevents that same F5 from
	// becoming the title screen's AnyKey and skipping the menu before the player sees it.
	if (!mInputArmed)
	{
		if (Input::GetKeyRelease(KeyCode::AnyKey))
			mInputArmed = true;

		return;
	}

	UpdateInputPresentation();

	if (mState == State::Attract)
	{
		if (Input::GetKeyTap(KeyCode::AnyKey))
			ShowState(State::Menu);

		return;
	}

	if (mState == State::Credits || mState == State::Settings)
	{
		if ((mBackButton && mBackButton->WasClicked()) || Input::GetActionTap("menu_back"))
		{
			ShowState(State::Menu);
			return;
		}

		if (mState == State::Settings
			&& ((mSoundButton && mSoundButton->WasClicked())
				|| Input::GetActionTap("menu_left") || Input::GetActionTap("menu_right")
				|| Input::GetActionTap("menu_confirm")))
		{
			mSoundEnabled = !mSoundEnabled;
			Audio::SetBusVolume(AudioBus::Master, mSoundEnabled ? 1.0f : 0.0f);
			ShowState(State::Settings);
		}

		return;
	}

	for (int32 index = 0; index < static_cast<int32>(mMenuButtons.size()); ++index)
	{
		Button* button = mMenuButtons[index];

		if (!button)
			continue;

		if (button->WasClicked())
		{
			mSelection = index;
			ActivateSelection();
			return;
		}

		if (button->IsHovered() && mSelection != index)
		{
			mSelection = index;
			RefreshMenu();
		}
	}

	if (Input::GetActionTap("menu_up"))
	{
		mSelection = (mSelection + 3) % 4;
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_down"))
	{
		mSelection = (mSelection + 1) % 4;
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_confirm"))
		ActivateSelection();
}

void MainMenu::ShowState(State state)
{
	mState = state;

	if (mPrompt)
	{
		mPrompt->SetVisible(state == State::Attract);
		mPrompt->SetEnabled(state == State::Attract);
	}
	if (mOptions)
	{
		mOptions->SetVisible(state == State::Menu);
		mOptions->SetEnabled(state == State::Menu);
	}
	if (mDetail)
	{
		const bool showDetail = state == State::Credits || state == State::Settings;
		mDetail->SetVisible(showDetail);
		mDetail->SetEnabled(showDetail);
	}
	if (mSoundButtonEntity)
	{
		mSoundButtonEntity->SetVisible(state == State::Settings);
		mSoundButtonEntity->SetEnabled(state == State::Settings);
	}
	if (mCreditsLogo)
	{
		mCreditsLogo->SetVisible(state == State::Credits);
		mCreditsLogo->SetEnabled(state == State::Credits);
	}

	if (state == State::Menu)
		RefreshMenu();
	else if (state == State::Credits && mDetailText)
		mDetailText->SetText("CREDITS\n\nKELLVYN SAMPAIO\nSAMPAIO GAMES STUDIO\nPOWERED BY LION ENGINE");
	else if (state == State::Settings && mDetailText)
		mDetailText->SetText("SETTINGS");

	if (state == State::Settings && mSoundButtonText)
		mSoundButtonText->SetText(mSoundEnabled ? "SOUND ON" : "SOUND OFF");

	UpdateInputPresentation(true);
}

void MainMenu::RefreshMenu()
{
	for (int32 index = 0; index < static_cast<int32>(mMenuButtons.size()); ++index)
		if (mMenuButtons[index])
			mMenuButtons[index]->SetSelected(index == mSelection);
}

void MainMenu::UpdateInputPresentation(bool force)
{
	const bool usingGamepad = Input::GetLastInputMethod() == InputMethod::Gamepad;

	if (!force && usingGamepad == mUsingGamepad)
		return;

	mUsingGamepad = usingGamepad;
	const auto show = [](Entity* entity, bool visible)
	{
		if (!entity)
			return;

		entity->SetVisible(visible);
		entity->SetEnabled(visible);
	};

	show(mPrompt, mState == State::Attract && !usingGamepad);
	show(mKeyboardControls, mState == State::Menu && !usingGamepad);
	show(mControllerAttractPrompt, mState == State::Attract && usingGamepad);
	show(mControllerMenuPrompts, mState == State::Menu && usingGamepad);
	show(mControllerDetailPrompts,
		(mState == State::Credits || mState == State::Settings) && usingGamepad);
	show(mControllerSettingsPrompt, mState == State::Settings && usingGamepad);
}

void MainMenu::ActivateSelection()
{
	switch (mSelection)
	{
		case 0: GameRules::StartNewGame(); break;
		case 1: ShowState(State::Credits); break;
		case 2: ShowState(State::Settings); break;
		case 3: Application::RequestQuit(); break;
	}
}

LION_REGISTER_COMPONENT(MainMenu)
