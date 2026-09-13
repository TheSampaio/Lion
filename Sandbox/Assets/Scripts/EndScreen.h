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

	void InitializeForScene();
};
