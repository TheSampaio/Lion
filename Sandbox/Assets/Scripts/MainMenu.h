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
	Lion::Entity* mSoundButtonEntity = nullptr;
	Lion::Entity* mCreditsLogo = nullptr;
	Lion::Entity* mKeyboardControls = nullptr;
	Lion::Entity* mControllerAttractPrompt = nullptr;
	Lion::Entity* mControllerMenuPrompts = nullptr;
	Lion::Entity* mControllerDetailPrompts = nullptr;
	Lion::Entity* mControllerSettingsPrompt = nullptr;
	Lion::TextRenderer* mDetailText = nullptr;
	Lion::TextRenderer* mSoundButtonText = nullptr;
	std::array<Lion::Button*, 4> mMenuButtons{};
	Lion::Button* mSoundButton = nullptr;
	Lion::Button* mBackButton = nullptr;
	Lion::int32 mSelection = 0;
	bool mInitialized = false;
	bool mInputArmed = false;
	bool mSoundEnabled = true;
	bool mUsingGamepad = false;

	void Initialize();
	void ShowState(State state);
	void RefreshMenu();
	void UpdateInputPresentation(bool force = false);
	void ActivateSelection();
};
