#include "MainMenu.h"
#include "GameAudio.h"
#include "GameProgress.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Render/Sprite.h>

using namespace Lion;

namespace
{
	constexpr int32 kLevelButtonsPerPage = 10;
	constexpr int32 kAchievementsPerPage = 5;

	void SetShown(Entity* entity, bool visible)
	{
		if (!entity)
			return;

		entity->SetVisible(visible);
		entity->SetEnabled(visible);
	}
}

void MainMenu::OnAwake()
{
	Window::SetBackgroundColor(0.015f, 0.02f, 0.045f);
	GameSettings::ApplyAudio();
	GameProgress::Load();
}

void MainMenu::Initialize()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> prompt = scene->FindEntity("Menu Prompt");
	const Reference<Entity> options = scene->FindEntity("Menu Options");
	const Reference<Entity> detail = scene->FindEntity("Menu Detail");
	const Reference<Entity> backButton = scene->FindEntity("Back Button");
	mPrompt = prompt.get();
	mOptions = options.get();
	mDetail = detail.get();
	mSettingsOptions = scene->FindEntity("Settings Options").get();
	mLevelOptions = scene->FindEntity("Level Options").get();
	mStatisticsPanel = scene->FindEntity("Statistics Panel").get();
	mAchievementsPanel = scene->FindEntity("Achievements Panel").get();
	mBackground = scene->FindEntity("Background").get();
	mBackButtonEntity = backButton.get();
	mCreditsLogo = scene->FindEntity("Credits Panel").get();
	mKeyboardControls = scene->FindEntity("Keyboard Menu Prompts").get();
	mKeyboardAttractPrompt = scene->FindEntity("Keyboard Attract Prompt").get();
	mKeyboardDetailPrompts = scene->FindEntity("Keyboard Detail Prompts").get();
	mKeyboardSettingsPrompt = scene->FindEntity("Keyboard Settings Prompt").get();
	mControllerAttractPrompt = scene->FindEntity("Controller Attract Prompt").get();
	mControllerMenuPrompts = scene->FindEntity("Controller Menu Prompts").get();
	mControllerDetailPrompts = scene->FindEntity("Controller Detail Prompts").get();
	mControllerSettingsPrompt = scene->FindEntity("Controller Settings Prompt").get();
	mKeyboardPagePrompt = scene->FindEntity("Keyboard Page Prompt").get();
	mControllerPagePrompt = scene->FindEntity("Controller Page Prompt").get();
	mDetailText = detail ? detail->GetComponent<TextRenderer>() : nullptr;
	mPromptText = prompt ? prompt->GetComponent<TextRenderer>() : nullptr;
	const Reference<Entity> statisticsText = scene->FindEntity("Statistics Text");
	const Reference<Entity> settingsPage = scene->FindEntity("Settings Page");
	const Reference<Entity> levelPage = scene->FindEntity("Level Page");
	const Reference<Entity> achievementProgress = scene->FindEntity("Achievement Progress");
	mStatisticsText = statisticsText ? statisticsText->GetComponent<TextRenderer>() : nullptr;
	mSettingsPageText = settingsPage ? settingsPage->GetComponent<TextRenderer>() : nullptr;
	mLevelPageText = levelPage ? levelPage->GetComponent<TextRenderer>() : nullptr;
	mAchievementProgressText = achievementProgress ? achievementProgress->GetComponent<TextRenderer>() : nullptr;
	mBackButton = backButton ? backButton->GetComponent<Button>() : nullptr;
	mBackButtonText = backButton ? backButton->GetComponent<TextRenderer>() : nullptr;

	static const char8* buttonNames[] = {
		"Continue Button", "Level Select Button", "Achievements Button", "Statistics Button",
		"Credits Button", "Settings Button", "Quit Button"
	};
	for (int32 index = 0; index < static_cast<int32>(mMenuButtons.size()); ++index)
	{
		const Reference<Entity> button = scene->FindEntity(buttonNames[index]);
		mMenuButtons[index] = button ? button->GetComponent<Button>() : nullptr;
		mMenuButtonTexts[index] = button ? button->GetComponent<TextRenderer>() : nullptr;
	}

	static const char8* settingNames[] = {
		"Resolution Button", "VSync Button", "Quality Button", "Bloom Button", "Vignette Button",
		"Motion Blur Button", "SFX Volume Button", "Music Volume Button", "Camera Shake Button",
		"Color Mode Button", "Language Button", "Control Hints Button"
	};
	for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
	{
		const Reference<Entity> button = scene->FindEntity(settingNames[index]);
		mSettingsButtons[index] = button ? button->GetComponent<Button>() : nullptr;
		mSettingsTexts[index] = button ? button->GetComponent<TextRenderer>() : nullptr;
		mSettingCombos[index] = button ? button->GetComponent<ComboBox>() : nullptr;
	}
	static const char8* checkBoxNames[] = {
		"", "VSync CheckBox", "", "Bloom CheckBox", "Vignette CheckBox", "Motion Blur CheckBox",
		"", "", "Camera Shake CheckBox", "", "", "Control Hints CheckBox"
	};
	for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
	{
		if (checkBoxNames[index][0] == '\0')
			continue;
		const Reference<Entity> checkBox = scene->FindEntity(checkBoxNames[index]);
		mSettingCheckBoxes[index] = checkBox ? checkBox->GetComponent<CheckBox>() : nullptr;
	}
	const Reference<Entity> sfxProgress = scene->FindEntity("SFX Progress");
	const Reference<Entity> musicProgress = scene->FindEntity("Music Progress");
	mSfxProgress = sfxProgress ? sfxProgress->GetComponent<ProgressBar>() : nullptr;
	mMusicProgress = musicProgress ? musicProgress->GetComponent<ProgressBar>() : nullptr;

	static const char8* groupNames[] = {
		"Graphics Settings", "Sound Settings", "Accessibility Settings", "Controls Settings"
	};
	for (int32 index = 0; index < static_cast<int32>(mSettingsGroups.size()); ++index)
	{
		mSettingsGroups[index] = scene->FindEntity(groupNames[index]).get();
		const Reference<Entity> tab = scene->FindEntity(LION_FORMAT_TEXT("Settings Tab {}", index + 1));
		mSettingsTabs[index] = tab ? tab->GetComponent<TextRenderer>() : nullptr;
	}

	for (int32 index = 0; index < kLevelButtonsPerPage; ++index)
	{
		const Reference<Entity> button = scene->FindEntity(LION_FORMAT_TEXT("Level Slot {} Button", index + 1));
		mLevelButtons[index] = button ? button->GetComponent<Button>() : nullptr;
		mLevelTexts[index] = button ? button->GetComponent<TextRenderer>() : nullptr;
	}

	for (int32 index = 0; index < kAchievementsPerPage; ++index)
	{
		mAchievementRows[index] = scene->FindEntity(LION_FORMAT_TEXT("Achievement Row {}", index + 1)).get();
		const Reference<Entity> icon = scene->FindEntity(LION_FORMAT_TEXT("Achievement Icon {}", index + 1));
		const Reference<Entity> text = scene->FindEntity(LION_FORMAT_TEXT("Achievement Text {}", index + 1));
		mAchievementIcons[index] = icon ? icon->GetComponent<SpriteRenderer>() : nullptr;
		mAchievementTexts[index] = text ? text->GetComponent<TextRenderer>() : nullptr;
	}

	RefreshLocalizedText();
	ShowState(State::Attract);
}

void MainMenu::OnUpdate()
{
	// OnAwake also runs while the editor is only displaying an authored scene. Audio belongs to the
	// running game, so it starts from the first simulation update instead.
	GameAudio::EnsureMusic(0);

	if (!mInitialized)
	{
		Initialize();
		return;
	}

	UpdateAmbientMotion();

	if (!mInputArmed)
	{
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
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
			mInputArmed = false;
		}
		return;
	}

	if (mState == State::Credits || mState == State::Statistics)
	{
		if ((mBackButton && mBackButton->WasClicked()) || Input::GetActionTap("menu_back"))
		{
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
		}
		return;
	}

	if (mState == State::Achievements)
	{
		if (Input::GetActionTap("menu_tab_left")) ChangeAchievementPage(-1);
		else if (Input::GetActionTap("menu_tab_right")) ChangeAchievementPage(1);
		else if ((mBackButton && mBackButton->WasClicked()) || Input::GetActionTap("menu_back"))
		{
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
		}
		return;
	}

	if (mState == State::Settings)
	{
		if (Input::GetActionTap("menu_back"))
		{
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
			return;
		}
		if (Input::GetActionTap("menu_tab_left")) { ChangeSettingsPage(-1); return; }
		if (Input::GetActionTap("menu_tab_right")) { ChangeSettingsPage(1); return; }

		for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
		{
			if (mSettingCombos[index] && mSettingCombos[index]->WasChanged())
			{
				GameSettings::SetValue(index, mSettingCombos[index]->GetSelectedIndex());
				mSettingsSelection = index;
				GameAudio::PlayUiSelect();
				if (index == 10) RefreshLocalizedText(); else RefreshSettings();
				return;
			}
			if (mSettingCheckBoxes[index] && mSettingCheckBoxes[index]->WasChanged())
			{
				GameSettings::SetValue(index, mSettingCheckBoxes[index]->IsChecked());
				mSettingsSelection = index;
				GameAudio::PlayUiSelect();
				RefreshSettings();
				if (index == 11)
					UpdateInputPresentation(true);
				return;
			}
		}
		if (mSfxProgress && mSfxProgress->WasChanged())
		{
			GameSettings::SetValue(6, static_cast<int32>(std::round(mSfxProgress->GetValue())));
			mSettingsSelection = 6;
			RefreshSettings();
			return;
		}
		if (mMusicProgress && mMusicProgress->WasChanged())
		{
			GameSettings::SetValue(7, static_cast<int32>(std::round(mMusicProgress->GetValue())));
			mSettingsSelection = 7;
			RefreshSettings();
			return;
		}

		for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
		{
			if (static_cast<int32>(GameSettings::GetCategory(index)) != mSettingsPage)
				continue;
			Button* button = mSettingsButtons[index];
			if (!button)
				continue;
			if (button->WasClicked())
			{
				mSettingsSelection = index;
				if (!mSettingCombos[index])
					ActivateSetting(1);
				else
					RefreshSettings();
				return;
			}
			if (button->IsHovered() && mSettingsSelection != index)
			{
				mSettingsSelection = index;
				GameAudio::PlayUiHover();
				RefreshSettings();
			}
		}

		if (mBackButton && mBackButton->WasClicked())
		{
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
			return;
		}
		if (mBackButton && mBackButton->IsHovered() && mSettingsSelection != GameSettings::kSettingCount)
		{
			mSettingsSelection = GameSettings::kSettingCount;
			GameAudio::PlayUiHover();
			RefreshSettings();
		}

		std::vector<int32> pageSettings;
		for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
			if (static_cast<int32>(GameSettings::GetCategory(index)) == mSettingsPage)
				pageSettings.push_back(index);
		if (Input::GetActionTap("menu_up") || Input::GetActionTap("menu_down"))
		{
			const int32 direction = Input::GetActionTap("menu_up") ? -1 : 1;
			auto found = std::find(pageSettings.begin(), pageSettings.end(), mSettingsSelection);
			if (mSettingsSelection == GameSettings::kSettingCount)
				mSettingsSelection = direction < 0 ? pageSettings.back() : pageSettings.front();
			else if (found != pageSettings.end())
			{
				const int32 position = static_cast<int32>(std::distance(pageSettings.begin(), found));
				const int32 next = position + direction;
				mSettingsSelection = next < 0 || next >= static_cast<int32>(pageSettings.size())
					? GameSettings::kSettingCount : pageSettings[next];
			}
			GameAudio::PlayUiHover();
			RefreshSettings();
		}
		else if (Input::GetActionTap("menu_left")) ActivateSetting(-1);
		else if (Input::GetActionTap("menu_right")) ActivateSetting(1);
		else if (Input::GetActionTap("menu_confirm"))
		{
			if (mSettingsSelection >= 0 && mSettingsSelection < GameSettings::kSettingCount
				&& mSettingCombos[mSettingsSelection])
				mSettingCombos[mSettingsSelection]->SetOpen(!mSettingCombos[mSettingsSelection]->IsOpen());
			else
				ActivateSetting(1);
		}
		return;
	}

	if (mState == State::LevelSelect)
	{
		if (Input::GetActionTap("menu_back"))
		{
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
			return;
		}
		if (Input::GetActionTap("menu_tab_left")) { ChangeLevelPage(-1); return; }
		if (Input::GetActionTap("menu_tab_right")) { ChangeLevelPage(1); return; }

		for (int32 index = 0; index < kLevelButtonsPerPage; ++index)
		{
			const int32 level = mLevelPage * kLevelButtonsPerPage + index + 1;
			Button* button = mLevelButtons[index];
			if (!button || !GameProgress::IsLevelUnlocked(level))
				continue;
			if (button->WasClicked())
			{
				GameAudio::PlayUiSelect();
				GameRules::StartAtLevel(level);
				return;
			}
			if (button->IsHovered() && mLevelSelection != index)
			{
				mLevelSelection = index;
				GameAudio::PlayUiHover();
				RefreshLevels();
			}
		}

		if (mBackButton && mBackButton->WasClicked())
		{
			GameAudio::PlayUiSelect();
			ShowState(State::Menu);
			return;
		}
		if (Input::GetActionTap("menu_up")) mLevelSelection = std::max(mLevelSelection - 2, 0);
		else if (Input::GetActionTap("menu_down")) mLevelSelection = std::min(mLevelSelection + 2, kLevelButtonsPerPage - 1);
		else if (Input::GetActionTap("menu_left") && (mLevelSelection % 2) == 1) mLevelSelection--;
		else if (Input::GetActionTap("menu_right") && (mLevelSelection % 2) == 0) mLevelSelection++;
		else if (Input::GetActionTap("menu_confirm"))
		{
			const int32 level = mLevelPage * kLevelButtonsPerPage + mLevelSelection + 1;
			if (GameProgress::IsLevelUnlocked(level))
			{
				GameAudio::PlayUiSelect();
				GameRules::StartAtLevel(level);
			}
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
			GameAudio::PlayUiSelect();
			ActivateSelection();
			return;
		}
		if (button->IsHovered() && mSelection != index)
		{
			mSelection = index;
			GameAudio::PlayUiHover();
			RefreshMenu();
		}
	}

	if (Input::GetActionTap("menu_up"))
	{
		mSelection = (mSelection + static_cast<int32>(mMenuButtons.size()) - 1)
			% static_cast<int32>(mMenuButtons.size());
		GameAudio::PlayUiHover();
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_down"))
	{
		mSelection = (mSelection + 1) % static_cast<int32>(mMenuButtons.size());
		GameAudio::PlayUiHover();
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_confirm"))
	{
		GameAudio::PlayUiSelect();
		ActivateSelection();
	}
}

void MainMenu::ShowState(State state)
{
	mState = state;
	SetShown(mOptions, state == State::Menu);
	SetShown(mSettingsOptions, state == State::Settings);
	SetShown(mLevelOptions, state == State::LevelSelect);
	SetShown(mStatisticsPanel, state == State::Statistics);
	SetShown(mAchievementsPanel, state == State::Achievements);
	SetShown(mBackButtonEntity, state != State::Attract && state != State::Menu);
	SetShown(mCreditsLogo, state == State::Credits);
	SetShown(mDetail, state != State::Attract && state != State::Menu);

	if (state == State::Menu)
		RefreshMenu();
	else if (state == State::Credits && mDetailText)
		mDetailText->SetText(GameSettings::Text(GameText::Credits));
	else if (state == State::Settings)
	{
		if (mDetailText) mDetailText->SetText(GameSettings::Text(GameText::Settings));
		mSettingsPage = 0;
		mSettingsSelection = 0;
		RefreshSettings();
	}
	else if (state == State::LevelSelect)
	{
		if (mDetailText) mDetailText->SetText(GameSettings::Text(GameText::LevelSelect));
		const int32 highest = std::max(GameProgress::GetHighestUnlockedLevel() - 1, 0);
		mLevelPage = highest / kLevelButtonsPerPage;
		mLevelSelection = highest % kLevelButtonsPerPage;
		RefreshLevels();
	}
	else if (state == State::Statistics)
	{
		if (mDetailText) mDetailText->SetText(GameSettings::Text(GameText::Statistics));
		RefreshStatistics();
	}
	else if (state == State::Achievements)
	{
		if (mDetailText) mDetailText->SetText("ACHIEVEMENTS");
		mAchievementPage = 0;
		RefreshAchievements();
	}

	UpdateInputPresentation(true);
}

void MainMenu::RefreshSettings()
{
	for (int32 category = 0; category < static_cast<int32>(mSettingsGroups.size()); ++category)
	{
		SetShown(mSettingsGroups[category], category == mSettingsPage);
		if (mSettingsTabs[category])
			mSettingsTabs[category]->SetText(category == mSettingsPage
				? LION_FORMAT_TEXT("[ {} ]", GameSettings::CategoryName(static_cast<GameSettings::Category>(category)))
				: GameSettings::CategoryName(static_cast<GameSettings::Category>(category)));
	}
	for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
	{
		if (mSettingCombos[index])
		{
			mSettingCombos[index]->SetPrefix(index == 0 ? GameSettings::Text(GameText::Resolution)
				: index == 2 ? GameSettings::Text(GameText::Quality)
				: index == 9 ? GameSettings::Text(GameText::ColorMode)
				: GameSettings::Text(GameText::Language));
			mSettingCombos[index]->SetSelectedIndex(GameSettings::GetValue(index));
		}
		else if (mSettingsTexts[index])
			mSettingsTexts[index]->SetText(GameSettings::Label(index));
		if (mSettingCheckBoxes[index])
			mSettingCheckBoxes[index]->SetChecked(GameSettings::GetValue(index) != 0);
		if (mSettingsButtons[index]) mSettingsButtons[index]->SetSelected(index == mSettingsSelection);
	}
	if (mSfxProgress) mSfxProgress->SetValue(static_cast<float32>(GameSettings::GetValue(6)));
	if (mMusicProgress) mMusicProgress->SetValue(static_cast<float32>(GameSettings::GetValue(7)));
	if (mSettingsPageText)
		mSettingsPageText->SetText(LION_FORMAT_TEXT("PAGE {}/4", mSettingsPage + 1));
	if (mBackButton) mBackButton->SetSelected(mSettingsSelection == GameSettings::kSettingCount);
}

void MainMenu::RefreshLevels()
{
	for (int32 index = 0; index < kLevelButtonsPerPage; ++index)
	{
		const int32 level = mLevelPage * kLevelButtonsPerPage + index + 1;
		const bool unlocked = GameProgress::IsLevelUnlocked(level);
		if (mLevelButtons[index])
		{
			mLevelButtons[index]->SetInteractable(unlocked);
			mLevelButtons[index]->SetSelected(unlocked && mLevelSelection == index);
		}
		if (!mLevelTexts[index])
			continue;
		if (!unlocked)
			mLevelTexts[index]->SetText(LION_FORMAT_TEXT("{} {:03}\n{}", GameSettings::Text(GameText::Level),
				level, GameSettings::Text(GameText::Locked)));
		else
			mLevelTexts[index]->SetText(LION_FORMAT_TEXT("{} {:03}{}\n{} {:06}", GameSettings::Text(GameText::Level),
				level, GameProgress::IsLevelCompleted(level) ? "  COMPLETE" : "",
				GameSettings::Text(GameText::HighScore), GameProgress::GetLevelHighScore(level)));
	}
	if (mLevelPageText)
		mLevelPageText->SetText(LION_FORMAT_TEXT("PAGE {:02}/10", mLevelPage + 1));
	if (mBackButton) mBackButton->SetSelected(false);
}

void MainMenu::RefreshStatistics()
{
	if (!mStatisticsText)
		return;
	const int32 seconds = GameProgress::GetPlayedSeconds();
	mStatisticsText->SetText(LION_FORMAT_TEXT(
		"{}                 {}/{}\n{}              {:08}\n{}                  {}\n{}            {}\n{}         {}\n{}              {}\n{}            X{}\n{}              {}\n{}               {:02}:{:02}:{:02}",
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

void MainMenu::RefreshAchievements()
{
	for (int32 row = 0; row < kAchievementsPerPage; ++row)
	{
		const int32 achievementIndex = mAchievementPage * kAchievementsPerPage + row;
		const GameProgress::Achievement& achievement = GameProgress::GetAchievement(achievementIndex);
		const bool unlocked = GameProgress::IsAchievementUnlocked(achievementIndex);
		SetShown(mAchievementRows[row], true);
		if (mAchievementIcons[row])
		{
			mAchievementIcons[row]->SetTexturePath(achievement.icon);
			mAchievementIcons[row]->GetSprite().SetColor(unlocked
				? Vector(1.0f, 1.0f, 1.0f) : Vector(0.22f, 0.28f, 0.38f));
		}
		if (mAchievementTexts[row])
		{
			mAchievementTexts[row]->SetText(LION_FORMAT_TEXT("{}\n{}", achievement.title, achievement.description));
			mAchievementTexts[row]->SetColor(unlocked
				? Vector(1.0f, 1.0f, 1.0f) : Vector(0.38f, 0.45f, 0.58f));
		}
	}
	if (mAchievementProgressText)
		mAchievementProgressText->SetText(LION_FORMAT_TEXT("{}/{} UNLOCKED   PAGE {}/2",
			GameProgress::GetUnlockedAchievementCount(), GameProgress::kAchievementCount, mAchievementPage + 1));
}

void MainMenu::RefreshLocalizedText()
{
	static const GameText menuText[] = {
		GameText::Continue, GameText::LevelSelect, GameText::Count, GameText::Statistics,
		GameText::Credits, GameText::Settings, GameText::Quit
	};
	for (int32 index = 0; index < static_cast<int32>(mMenuButtonTexts.size()); ++index)
		if (mMenuButtonTexts[index])
			mMenuButtonTexts[index]->SetText(index == 0 && GameProgress::GetCompletedLevelCount() == 0
				? GameSettings::Text(GameText::Play)
				: index == 2 ? "ACHIEVEMENTS" : GameSettings::Text(menuText[index]));
	if (mBackButtonText) mBackButtonText->SetText(GameSettings::Text(GameText::Back));
	if (mPromptText) mPromptText->SetText(GameSettings::Text(GameText::PressAny));
	RefreshSettings();
	RefreshLevels();
	if (mState == State::Statistics) RefreshStatistics();
	if (mState == State::Achievements) RefreshAchievements();
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
	SetShown(mPrompt, mState == State::Attract && !hints);
	SetShown(mKeyboardAttractPrompt, hints && mState == State::Attract && !usingGamepad);
	SetShown(mControllerAttractPrompt, hints && mState == State::Attract && usingGamepad);
	SetShown(mKeyboardControls, hints && mState == State::Menu && !usingGamepad);
	SetShown(mControllerMenuPrompts, hints && mState == State::Menu && usingGamepad);
	const bool detail = mState != State::Attract && mState != State::Menu;
	const bool paged = mState == State::LevelSelect || mState == State::Achievements;
	SetShown(mKeyboardDetailPrompts, hints && detail && !usingGamepad);
	SetShown(mControllerDetailPrompts, hints && detail && usingGamepad);
	SetShown(mKeyboardSettingsPrompt, hints && mState == State::Settings && !usingGamepad);
	SetShown(mControllerSettingsPrompt, hints && mState == State::Settings && usingGamepad);
	SetShown(mKeyboardPagePrompt, hints && paged && !usingGamepad);
	SetShown(mControllerPagePrompt, hints && paged && usingGamepad);
}

void MainMenu::UpdateAmbientMotion()
{
	if (!mBackground)
		return;

	mAmbientTime += Clock::GetDeltaTime();
	const float32 pulse = 1.02f + std::sin(mAmbientTime * 0.23f) * 0.012f;
	mBackground->GetTransform()->SetScale(Vector2(pulse, pulse));
	mBackground->GetTransform()->SetRotation(std::sin(mAmbientTime * 0.13f) * 0.18f);
}

void MainMenu::ActivateSelection()
{
	switch (mSelection)
	{
		case 0: GameRules::ContinueGame(); break;
		case 1: ShowState(State::LevelSelect); break;
		case 2: ShowState(State::Achievements); break;
		case 3: ShowState(State::Statistics); break;
		case 4: ShowState(State::Credits); break;
		case 5: ShowState(State::Settings); break;
		case 6: Application::RequestQuit(); break;
	}
}

void MainMenu::ActivateSetting(int32 direction)
{
	if (mSettingsSelection == GameSettings::kSettingCount)
	{
		ShowState(State::Menu);
		return;
	}

	GameAudio::PlayUiSelect();
	if (mSettingCombos[mSettingsSelection])
	{
		mSettingCombos[mSettingsSelection]->SelectRelative(direction);
		GameSettings::SetValue(mSettingsSelection,
			mSettingCombos[mSettingsSelection]->GetSelectedIndex());
	}
	else
		GameSettings::Change(mSettingsSelection, direction);
	if (mSettingsSelection == 10)
		RefreshLocalizedText();
	else
		RefreshSettings();
	UpdateInputPresentation(true);

	const Reference<Scene> scene = GetOwner().GetScene();
	if (PostProcessingComponent* postProcessing = scene ? scene->FindComponent<PostProcessingComponent>() : nullptr)
		GameSettings::Apply(*postProcessing);
}

void MainMenu::ChangeSettingsPage(int32 direction)
{
	const int32 count = static_cast<int32>(GameSettings::Category::Count);
	mSettingsPage = (mSettingsPage + direction + count) % count;
	for (int32 index = 0; index < GameSettings::kSettingCount; ++index)
		if (static_cast<int32>(GameSettings::GetCategory(index)) == mSettingsPage)
		{
			mSettingsSelection = index;
			break;
		}
	GameAudio::PlayUiHover();
	RefreshSettings();
}

void MainMenu::ChangeLevelPage(int32 direction)
{
	mLevelPage = (mLevelPage + direction + 10) % 10;
	mLevelSelection = 0;
	GameAudio::PlayUiHover();
	RefreshLevels();
}

void MainMenu::ChangeAchievementPage(int32 direction)
{
	mAchievementPage = (mAchievementPage + direction + 2) % 2;
	GameAudio::PlayUiHover();
	RefreshAchievements();
}

LION_REGISTER_COMPONENT(MainMenu)
