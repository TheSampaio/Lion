#pragma once

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
	};

	State mState = State::Attract;
	Lion::Entity* mPrompt = nullptr;
	Lion::Entity* mOptions = nullptr;
	Lion::Entity* mDetail = nullptr;
	Lion::Entity* mSettingsOptions = nullptr;
	Lion::Entity* mLevelOptions = nullptr;
	Lion::Entity* mStatisticsPanel = nullptr;
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
	std::array<Lion::Button*, 6> mMenuButtons{};
	std::array<Lion::TextRenderer*, 6> mMenuButtonTexts{};
	std::array<Lion::Button*, 11> mSettingsButtons{};
	std::array<Lion::TextRenderer*, 11> mSettingsTexts{};
	std::array<Lion::Button*, 5> mLevelButtons{};
	std::array<Lion::TextRenderer*, 5> mLevelTexts{};
	Lion::Button* mBackButton = nullptr;
	Lion::TextRenderer* mBackButtonText = nullptr;
	Lion::int32 mSelection = 0;
	Lion::int32 mSettingsSelection = 0;
	Lion::int32 mLevelSelection = 0;
	bool mInitialized = false;
	bool mInputArmed = false;
	bool mUsingGamepad = false;

	void Initialize();
	void ShowState(State state);
	void RefreshMenu();
	void RefreshSettings();
	void RefreshLevels();
	void RefreshStatistics();
	void RefreshLocalizedText();
	void UpdateInputPresentation(bool force = false);
	void ActivateSelection();
	void ActivateSetting(Lion::int32 direction);
};
