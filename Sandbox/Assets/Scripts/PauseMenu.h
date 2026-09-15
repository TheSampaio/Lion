#pragma once

#include <Lion/Lion.h>

// Owns the in-game pause overlay while SceneManager freezes ordinary gameplay and physics.
class PauseMenu final : public Lion::Component
{
public:
	void OnUpdate() override;
	void OnDestroy() override;
	bool UpdatesWhenPaused() const override { return true; }

private:
	bool mInitialized = false;
	Lion::Entity* mOverlay = nullptr;
	Lion::Entity* mControllerPrompts = nullptr;
	Lion::Entity* mKeyboardPrompts = nullptr;
	Lion::Button* mResumeButton = nullptr;
	Lion::Button* mMainMenuButton = nullptr;
	Lion::int32 mSelection = 0;
	bool mShowingGamepadPrompts = false;
	bool mShowingKeyboardPrompts = false;

	void Initialize();
	void Show(bool visible);
	void RefreshSelection();
	void UpdateInputPrompts();
	void ActivateSelection();
};
