#pragma once

#include <Lion/Lion.h>

// Presents the shared arcade result screen. Its scene path decides victory or defeat; the session score
// always comes from the single GameRules controller.
class EndScreen final : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;

private:
	bool mInitialized = false;
	bool mInputArmed = false;
	bool mShowingControllerPrompts = false;
	Lion::Button* mPlayAgainButton = nullptr;
	Lion::Button* mMainMenuButton = nullptr;
	Lion::Entity* mControllerPrompts = nullptr;
	Lion::int32 mSelection = 0;

	void InitializeForScene();
	void RefreshSelection();
	void UpdateControllerPrompts();
	void ActivateSelection();
};
