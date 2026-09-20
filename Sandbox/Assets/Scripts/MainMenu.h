#pragma once

#include "GameSettings.h"

#include <Lion/Lion.h>

#include <array>

// Drives the title screen and its pointer/keyboard/gamepad menu without owning game-session state.
class MainMenu final : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;

private:
	enum class State
	{
		Attract,
		Menu,
		Credits,
		Settings,
		LevelSelect,
		Statistics,
		Achievements,
	};

	State mState = State::Attract;
	Lion::Entity* mPrompt = nullptr;
	Lion::Entity* mOptions = nullptr;
	Lion::Entity* mDetail = nullptr;
	Lion::Entity* mSettingsOptions = nullptr;
	Lion::Entity* mLevelOptions = nullptr;
	Lion::Entity* mStatisticsPanel = nullptr;
	Lion::Entity* mAchievementsPanel = nullptr;
	Lion::Entity* mBackground = nullptr;
	Lion::Entity* mBackButtonEntity = nullptr;
	Lion::Entity* mCreditsLogo = nullptr;
	Lion::Entity* mKeyboardControls = nullptr;
	Lion::Entity* mKeyboardAttractPrompt = nullptr;
	Lion::Entity* mKeyboardDetailPrompts = nullptr;
	Lion::Entity* mKeyboardSettingsPrompt = nullptr;
	Lion::Entity* mControllerAttractPrompt = nullptr;
	Lion::Entity* mControllerMenuPrompts = nullptr;
	Lion::Entity* mControllerDetailPrompts = nullptr;
	Lion::Entity* mControllerSettingsPrompt = nullptr;
	Lion::TextRenderer* mDetailText = nullptr;
	Lion::TextRenderer* mPromptText = nullptr;
	Lion::TextRenderer* mStatisticsText = nullptr;
	Lion::TextRenderer* mSettingsPageText = nullptr;
	Lion::TextRenderer* mLevelPageText = nullptr;
	Lion::TextRenderer* mAchievementProgressText = nullptr;
	std::array<Lion::Button*, 7> mMenuButtons{};
	std::array<Lion::TextRenderer*, 7> mMenuButtonTexts{};
	std::array<Lion::Button*, GameSettings::kSettingCount> mSettingsButtons{};
	std::array<Lion::TextRenderer*, GameSettings::kSettingCount> mSettingsTexts{};
	std::array<Lion::ComboBox*, GameSettings::kSettingCount> mSettingCombos{};
	std::array<Lion::CheckBox*, GameSettings::kSettingCount> mSettingCheckBoxes{};
	std::array<Lion::Entity*, static_cast<size_t>(GameSettings::Category::Count)> mSettingsGroups{};
	std::array<Lion::TextRenderer*, static_cast<size_t>(GameSettings::Category::Count)> mSettingsTabs{};
	std::array<Lion::Button*, 10> mLevelButtons{};
	std::array<Lion::TextRenderer*, 10> mLevelTexts{};
	std::array<Lion::Entity*, 5> mAchievementRows{};
	std::array<Lion::SpriteRenderer*, 5> mAchievementIcons{};
	std::array<Lion::TextRenderer*, 5> mAchievementTexts{};
	Lion::Button* mBackButton = nullptr;
	Lion::TextRenderer* mBackButtonText = nullptr;
	Lion::ProgressBar* mSfxProgress = nullptr;
	Lion::ProgressBar* mMusicProgress = nullptr;
	Lion::int32 mSelection = 0;
	Lion::int32 mSettingsSelection = 0;
	Lion::int32 mLevelSelection = 0;
	Lion::int32 mSettingsPage = 0;
	Lion::int32 mLevelPage = 0;
	Lion::int32 mAchievementPage = 0;
	Lion::float32 mAmbientTime = 0.0f;
	bool mInitialized = false;
	bool mInputArmed = false;
	bool mUsingGamepad = false;

	void Initialize();
	void ShowState(State state);
	void RefreshMenu();
	void RefreshSettings();
	void RefreshLevels();
	void RefreshStatistics();
	void RefreshAchievements();
	void RefreshLocalizedText();
	void UpdateInputPresentation(bool force = false);
	void UpdateAmbientMotion();
	void ActivateSelection();
	void ActivateSetting(Lion::int32 direction);
	void ChangeSettingsPage(Lion::int32 direction);
	void ChangeLevelPage(Lion::int32 direction);
	void ChangeAchievementPage(Lion::int32 direction);
};
