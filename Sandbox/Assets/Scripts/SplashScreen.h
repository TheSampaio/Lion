#pragma once

#include <Lion/Lion.h>

// Holds the engine mark long enough to read before the shared transition enters the title screen.
class SplashScreen final : public Lion::Component
{
public:
	void OnUpdate() override;

private:
	Lion::float32 mElapsed = 0.0f;
	bool mQueued = false;
};
