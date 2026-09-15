#include "MainMenu.h"
#include "GameProgress.h"
#include "GameRules.h"
#include "GameSettings.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void MainMenu::OnAwake()
{
	Window::SetBackgroundColor(0.015f, 0.02f, 0.045f);
	GameProgress::Load();
}

void MainMenu::Initialize()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> prompt = scene->FindEntity("Menu Prompt");
	const Reference<Entity> options = scene->FindEntity("Menu Options");
	const Reference<Entity> detail = scene->FindEntity("Menu Detail");
	const Reference<Entity> settingsOptions = scene->FindEntity("Settings Options");
	const Reference<Entity> levelOptions = scene->FindEntity("Level Options");
	const Reference<Entity> statisticsPanel = scene->FindEntity("Statistics Panel");
	const Reference<Entity> statisticsText = scene->FindEntity("Statistics Text");
	const Reference<Entity> backButton = scene->FindEntity("Back Button");
	const Reference<Entity> creditsLogo = scene->FindEntity("Credits Logo");
	mPrompt = prompt.get();
	mOptions = options.get();
	mDetail = detail.get();
	mSettingsOptions = settingsOptions.get();
	mLevelOptions = levelOptions.get();
	mStatisticsPanel = statisticsPanel.get();
	mBackButtonEntity = backButton.get();
	mCreditsLogo = creditsLogo.get();
	mKeyboardControls = scene->FindEntity("Keyboard Menu Prompts").get();
	mKeyboardAttractPrompt = scene->FindEntity("Keyboard Attract Prompt").get();
	mKeyboardDetailPrompts = scene->FindEntity("Keyboard Detail Prompts").get();
	mKeyboardSettingsPrompt = scene->FindEntity("Keyboard Settings Prompt").get();
	mControllerAttractPrompt = scene->FindEntity("Controller Attract Prompt").get();
	mControllerMenuPrompts = scene->FindEntity("Controller Menu Prompts").get();
	mControllerDetailPrompts = scene->FindEntity("Controller Detail Prompts").get();
	mControllerSettingsPrompt = scene->FindEntity("Controller Settings Prompt").get();
	mDetailText = detail ? detail->GetComponent<TextRenderer>() : nullptr;
	mPromptText = prompt ? prompt->GetComponent<TextRenderer>() : nullptr;
	mStatisticsText = statisticsText ? statisticsText->GetComponent<TextRenderer>() : nullptr;
	mBackButton = backButton ? backButton->GetComponent<Button>() : nullptr;
	mBackButtonText = backButton ? backButton->GetComponent<TextRenderer>() : nullptr;

	static const char8* buttonNames[] = {
		"Continue Button", "Level Select Button", "Statistics Button",
		"Credits Button", "Settings Button", "Quit Button"
	};
	for (int32 index = 0; index < static_cast<int32>(mMenuButtons.size()); ++index)
	{
		const Reference<Entity> button = scene->FindEntity(buttonNames[index]);
		mMenuButtons[index] = button ? button->GetComponent<Button>() : nullptr;
		mMenuButtonTexts[index] = button ? button->GetComponent<TextRenderer>() : nullptr;
	}

	static const char8* settingNames[] = {
		"Sound Button", "Resolution Button", "VSync Button", "Quality Button", "Bloom Button",
		"Vignette Button", "Motion Blur Button", "Camera Shake Button", "Color Mode Button",
		"Language Button", "Control Hints Button"
	};
	for (int32 index = 0; index < static_cast<int32>(mSettingsButtons.size()); ++index)
	{
		const Reference<Entity> button = scene->FindEntity(settingNames[index]);
		mSettingsButtons[index] = button ? button->GetComponent<Button>() : nullptr;
		mSettingsTexts[index] = button ? button->GetComponent<TextRenderer>() : nullptr;
	}

	for (int32 index = 0; index < static_cast<int32>(mLevelButtons.size()); ++index)
	{
		const Reference<Entity> button = scene->FindEntity(LION_FORMAT_TEXT("Level {} Button", index + 1));
		mLevelButtons[index] = button ? button->GetComponent<Button>() : nullptr;
		mLevelTexts[index] = button ? button->GetComponent<TextRenderer>() : nullptr;
	}

	RefreshLocalizedText();
	ShowState(State::Attract);
}

void MainMenu::OnUpdate()
{
	if (!mInitialized)
	{
		Initialize();
		return;
	}

	if (!mInputArmed)
	{
		// Do not consume the input that dismissed the splash screen. Once every confirm source has
		// been released, keyboard, pointer and controller may all enter the menu on the next press.
		if (!Input::GetKeyPress(KeyCode::AnyKey) && !Input::GetMouseButtonPress(0)
			&& Input::GetActionStrength("menu_confirm") <= 0.0f)
			mInputArmed = true;
		return;
	}

	UpdateInputPresentation();

	if (mState == State::Attract)
	{
		if (Input::GetKeyTap(KeyCode::AnyKey) || Input::GetMouseButtonPress(0)
			|| Input::GetActionTap("menu_confirm"))
		{
			ShowState(State::Menu);
			mInputArmed = false;
		}
		return;
	}

	if (mState == State::Credits || mState == State::Statistics)
	{
		if ((mBackButton && mBackButton->WasClicked()) || Input::GetActionTap("menu_back"))
			ShowState(State::Menu);
		return;
	}

	if (mState == State::Settings)
	{
		if (Input::GetActionTap("menu_back"))
		{
			ShowState(State::Menu);
			return;
		}

		for (int32 index = 0; index < static_cast<int32>(mSettingsButtons.size()); ++index)
		{
			Button* button = mSettingsButtons[index];
			if (!button)
				continue;
			if (button->WasClicked())
			{
				mSettingsSelection = index;
				ActivateSetting(1);
				return;
			}
			if (button->IsHovered() && mSettingsSelection != index)
			{
				mSettingsSelection = index;
				RefreshSettings();
			}
		}

		if (mBackButton && mBackButton->WasClicked())
		{
			ShowState(State::Menu);
			return;
		}
		if (mBackButton && mBackButton->IsHovered() && mSettingsSelection != GameSettings::kSettingCount)
		{
			mSettingsSelection = GameSettings::kSettingCount;
			RefreshSettings();
		}

		if (Input::GetActionTap("menu_up"))
		{
			mSettingsSelection = (mSettingsSelection + GameSettings::kSettingCount)
				% (GameSettings::kSettingCount + 1);
			RefreshSettings();
		}
		else if (Input::GetActionTap("menu_down"))
		{
			mSettingsSelection = (mSettingsSelection + 1) % (GameSettings::kSettingCount + 1);
			RefreshSettings();
		}
		else if (Input::GetActionTap("menu_left")) ActivateSetting(-1);
		else if (Input::GetActionTap("menu_right") || Input::GetActionTap("menu_confirm")) ActivateSetting(1);
		return;
	}

	if (mState == State::LevelSelect)
	{
		if (Input::GetActionTap("menu_back"))
		{
			ShowState(State::Menu);
			return;
		}

		for (int32 index = 0; index < static_cast<int32>(mLevelButtons.size()); ++index)
		{
			Button* button = mLevelButtons[index];
			if (!button || !GameProgress::IsLevelUnlocked(index + 1))
				continue;
			if (button->WasClicked())
			{
				GameRules::StartAtLevel(index + 1);
				return;
			}
			if (button->IsHovered() && mLevelSelection != index)
			{
				mLevelSelection = index;
				RefreshLevels();
			}
		}

		if (mBackButton && mBackButton->WasClicked())
		{
			ShowState(State::Menu);
			return;
		}
		if (mBackButton && mBackButton->IsHovered() && mLevelSelection != GameProgress::kLevelCount)
		{
			mLevelSelection = GameProgress::kLevelCount;
			RefreshLevels();
		}

		const int32 lastUnlocked = GameProgress::GetHighestUnlockedLevel() - 1;
		if (Input::GetActionTap("menu_up"))
			mLevelSelection = mLevelSelection == GameProgress::kLevelCount
				? lastUnlocked : (mLevelSelection > 0 ? mLevelSelection - 1 : GameProgress::kLevelCount);
		else if (Input::GetActionTap("menu_down"))
			mLevelSelection = mLevelSelection == GameProgress::kLevelCount
				? 0 : (mLevelSelection < lastUnlocked ? mLevelSelection + 1 : GameProgress::kLevelCount);
		else if (Input::GetActionTap("menu_confirm"))
		{
			if (mLevelSelection == GameProgress::kLevelCount) ShowState(State::Menu);
			else GameRules::StartAtLevel(mLevelSelection + 1);
			return;
		}
		RefreshLevels();
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
		mSelection = (mSelection + static_cast<int32>(mMenuButtons.size()) - 1)
			% static_cast<int32>(mMenuButtons.size());
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_down"))
	{
		mSelection = (mSelection + 1) % static_cast<int32>(mMenuButtons.size());
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_confirm")) ActivateSelection();
}

void MainMenu::ShowState(State state)
{
	mState = state;
	const auto show = [](Entity* entity, bool visible)
	{
		if (!entity) return;
		entity->SetVisible(visible);
		entity->SetEnabled(visible);
	};

	show(mOptions, state == State::Menu);
	show(mSettingsOptions, state == State::Settings);
	show(mLevelOptions, state == State::LevelSelect);
	show(mStatisticsPanel, state == State::Statistics);
	show(mBackButtonEntity, state == State::Credits || state == State::Settings
		|| state == State::LevelSelect || state == State::Statistics);
	show(mCreditsLogo, state == State::Credits);
	show(mDetail, state == State::Credits || state == State::Settings
		|| state == State::LevelSelect || state == State::Statistics);

	if (state == State::Menu)
		RefreshMenu();
	else if (state == State::Credits && mDetailText)
		mDetailText->SetText(GameSettings::Text(GameText::Credits));
	else if (state == State::Settings)
	{
		if (mDetailText) mDetailText->SetText(GameSettings::Text(GameText::Settings));
		mSettingsSelection = 0;
		RefreshSettings();
	}
	else if (state == State::LevelSelect)
	{
		if (mDetailText) mDetailText->SetText(GameSettings::Text(GameText::LevelSelect));
		mLevelSelection = std::max(GameProgress::GetHighestUnlockedLevel() - 1, 0);
		RefreshLevels();
	}
	else if (state == State::Statistics)
	{
		if (mDetailText) mDetailText->SetText(GameSettings::Text(GameText::Statistics));
		RefreshStatistics();
	}

	UpdateInputPresentation(true);
}

void MainMenu::RefreshSettings()
{
	for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
	{
		if (mSettingsTexts[index]) mSettingsTexts[index]->SetText(GameSettings::Label(index));
		if (mSettingsButtons[index]) mSettingsButtons[index]->SetSelected(index == mSettingsSelection);
	}
	if (mBackButton) mBackButton->SetSelected(mSettingsSelection == GameSettings::kSettingCount);
}

void MainMenu::RefreshLevels()
{
	for (int32 index = 0; index < static_cast<int32>(mLevelButtons.size()); ++index)
	{
		const int32 level = index + 1;
		const bool unlocked = GameProgress::IsLevelUnlocked(level);
		if (mLevelButtons[index])
		{
			mLevelButtons[index]->SetInteractable(unlocked);
			mLevelButtons[index]->SetSelected(unlocked && mLevelSelection == index);
		}
		if (!mLevelTexts[index])
			continue;
		if (!unlocked)
			mLevelTexts[index]->SetText(LION_FORMAT_TEXT("{} {:02}  {}", GameSettings::Text(GameText::Level),
				level, GameSettings::Text(GameText::Locked)));
		else
			mLevelTexts[index]->SetText(LION_FORMAT_TEXT("{} {:02}  {} {:06}{}", GameSettings::Text(GameText::Level),
				level, GameSettings::Text(GameText::HighScore), GameProgress::GetLevelHighScore(level),
				GameProgress::IsLevelCompleted(level) ? "  *" : ""));
	}
	if (mBackButton) mBackButton->SetSelected(mLevelSelection == GameProgress::kLevelCount);
}

void MainMenu::RefreshStatistics()
{
	if (!mStatisticsText)
		return;
	const int32 seconds = GameProgress::GetPlayedSeconds();
	mStatisticsText->SetText(LION_FORMAT_TEXT(
		"{}  {}/{}\n{}  {:08}\n{}  {}\n{}  {}\n{}  {}\n{}  {}\n{}  X{}\n{}  {}\n{}  {:02}:{:02}:{:02}",
		GameSettings::Text(GameText::Completed), GameProgress::GetCompletedLevelCount(), GameProgress::kLevelCount,
		GameSettings::Text(GameText::TotalScore), GameProgress::GetTotalScore(),
		GameSettings::Text(GameText::Sessions), GameProgress::GetSessionsPlayed(),
		GameSettings::Text(GameText::LevelsCleared), GameProgress::GetLevelsCompleted(),
		GameSettings::Text(GameText::BricksDestroyed), GameProgress::GetBricksDestroyed(),
		GameSettings::Text(GameText::BallsLost), GameProgress::GetBallsLost(),
		GameSettings::Text(GameText::HighestCombo), GameProgress::GetHighestCombo(),
		GameSettings::Text(GameText::Shockwaves), GameProgress::GetShockwavesFired(),
		GameSettings::Text(GameText::PlayTime), seconds / 3600, seconds / 60 % 60, seconds % 60));
}

void MainMenu::RefreshLocalizedText()
{
	static const GameText menuText[] = {
		GameText::Continue, GameText::LevelSelect, GameText::Statistics,
		GameText::Credits, GameText::Settings, GameText::Quit
	};
	for (int32 index = 0; index < static_cast<int32>(mMenuButtonTexts.size()); ++index)
		if (mMenuButtonTexts[index])
			mMenuButtonTexts[index]->SetText(index == 0 && GameProgress::GetCompletedLevelCount() == 0
				? GameSettings::Text(GameText::Play) : GameSettings::Text(menuText[index]));
	if (mBackButtonText) mBackButtonText->SetText(GameSettings::Text(GameText::Back));
	if (mPromptText) mPromptText->SetText(GameSettings::Text(GameText::PressAny));
	RefreshSettings();
	RefreshLevels();
	if (mState == State::Statistics) RefreshStatistics();
}

void MainMenu::RefreshMenu()
{
	for (int32 index = 0; index < static_cast<int32>(mMenuButtons.size()); ++index)
		if (mMenuButtons[index]) mMenuButtons[index]->SetSelected(index == mSelection);
}

void MainMenu::UpdateInputPresentation(bool force)
{
	const bool usingGamepad = Input::GetLastInputMethod() == InputMethod::Gamepad;
	if (!force && usingGamepad == mUsingGamepad)
		return;

	mUsingGamepad = usingGamepad;
	const bool hints = GameSettings::HasControlHints();
	const bool detail = mState == State::Credits || mState == State::Settings
		|| mState == State::LevelSelect || mState == State::Statistics;
	const auto show = [](Entity* entity, bool visible)
	{
		if (!entity) return;
		entity->SetVisible(visible);
		entity->SetEnabled(visible);
	};

	show(mPrompt, mState == State::Attract && !hints);
	show(mKeyboardAttractPrompt, hints && mState == State::Attract && !usingGamepad);
	show(mControllerAttractPrompt, hints && mState == State::Attract && usingGamepad);
	show(mKeyboardControls, hints && mState == State::Menu && !usingGamepad);
	show(mControllerMenuPrompts, hints && mState == State::Menu && usingGamepad);
	show(mKeyboardDetailPrompts, hints && detail && !usingGamepad);
	show(mControllerDetailPrompts, hints && detail && usingGamepad);
	show(mKeyboardSettingsPrompt, hints && mState == State::Settings && !usingGamepad);
	show(mControllerSettingsPrompt, hints && mState == State::Settings && usingGamepad);
}

void MainMenu::ActivateSelection()
{
	switch (mSelection)
	{
		case 0: GameRules::ContinueGame(); break;
		case 1: ShowState(State::LevelSelect); break;
		case 2: ShowState(State::Statistics); break;
		case 3: ShowState(State::Credits); break;
		case 4: ShowState(State::Settings); break;
		case 5: Application::RequestQuit(); break;
	}
}

void MainMenu::ActivateSetting(int32 direction)
{
	if (mSettingsSelection == GameSettings::kSettingCount)
	{
		ShowState(State::Menu);
		return;
	}

	GameSettings::Change(mSettingsSelection, direction);
	if (mSettingsSelection == 9)
		RefreshLocalizedText();
	else
		RefreshSettings();
	UpdateInputPresentation(true);

	const Reference<Scene> scene = GetOwner().GetScene();
	if (PostProcessingComponent* postProcessing = scene ? scene->FindComponent<PostProcessingComponent>() : nullptr)
		GameSettings::Apply(*postProcessing);
}

LION_REGISTER_COMPONENT(MainMenu)
