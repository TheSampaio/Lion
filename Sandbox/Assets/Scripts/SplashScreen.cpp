#include "SplashScreen.h"
#include "ScreenTransition.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void SplashScreen::OnUpdate()
{
	mElapsed += Clock::GetDeltaTime();
	if (!mQueued && mElapsed >= 1.35f)
	{
		mQueued = true;
		ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
	}
}

LION_REGISTER_COMPONENT(SplashScreen)
