#pragma once

#include <Lion/Lion.h>

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
	Lion::TextRenderer* mOptionsText = nullptr;
	Lion::TextRenderer* mDetailText = nullptr;
	Lion::int32 mSelection = 0;
	bool mInputArmed = false;
	bool mSoundEnabled = true;

	void ShowState(State state);
	void RefreshMenu();
	void ActivateSelection();
};
