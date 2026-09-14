#pragma once

#include <Lion/Lion.h>

#include <array>

// Drives the title screen and its keyboard/gamepad menu without owning any game-session state.
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
	};

	State mState = State::Attract;
	Lion::Entity* mPrompt = nullptr;
	Lion::Entity* mOptions = nullptr;
	Lion::Entity* mDetail = nullptr;
	Lion::Entity* mSettingsOptions = nullptr;
	Lion::Entity* mBackButtonEntity = nullptr;
	Lion::Entity* mCreditsLogo = nullptr;
	Lion::Entity* mKeyboardControls = nullptr;
	Lion::Entity* mControllerAttractPrompt = nullptr;
	Lion::Entity* mControllerMenuPrompts = nullptr;
	Lion::Entity* mControllerDetailPrompts = nullptr;
	Lion::Entity* mControllerSettingsPrompt = nullptr;
	Lion::TextRenderer* mDetailText = nullptr;
	Lion::TextRenderer* mPromptText = nullptr;
	std::array<Lion::Button*, 4> mMenuButtons{};
	std::array<Lion::TextRenderer*, 4> mMenuButtonTexts{};
	std::array<Lion::Button*, 10> mSettingsButtons{};
	std::array<Lion::TextRenderer*, 10> mSettingsTexts{};
	Lion::Button* mBackButton = nullptr;
	Lion::TextRenderer* mBackButtonText = nullptr;
	Lion::int32 mSelection = 0;
	Lion::int32 mSettingsSelection = 0;
	bool mInitialized = false;
	bool mInputArmed = false;
	bool mUsingGamepad = false;

	void Initialize();
	void ShowState(State state);
	void RefreshMenu();
	void RefreshSettings();
	void RefreshLocalizedText();
	void UpdateInputPresentation(bool force = false);
	void ActivateSelection();
	void ActivateSetting(Lion::int32 direction);
};
