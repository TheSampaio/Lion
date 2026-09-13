#include "EndScreen.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void EndScreen::OnAwake()
{
	// Scene path-dependent presentation is initialized on the first update; SceneManager publishes the
	// destination path only after all destination entities have Awakened.
}

void EndScreen::InitializeForScene()
{
	mInitialized = true;
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> titleEntity = scene->FindEntity("Result Title");
	const Reference<Entity> scoreEntity = scene->FindEntity("Result Score");
	const bool victory = SceneManager::GetActivePath().find("Victory") != std::string::npos;

	if (TextRenderer* title = titleEntity ? titleEntity->GetComponent<TextRenderer>() : nullptr)
		title->SetText(victory ? "YOU WIN!" : "GAME OVER");

	if (TextRenderer* score = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr)
		score->SetText(LION_FORMAT_TEXT("TOTAL SCORE\n{:06}", GameRules::GetScore()));

	Window::SetBackgroundColor(victory ? 0.015f : 0.08f, victory ? 0.08f : 0.015f, 0.035f);
}

void EndScreen::OnUpdate()
{
	if (!mInitialized)
	{
		InitializeForScene();
		return;
	}

	if (!mInputArmed)
	{
		if (Input::GetKeyRelease(KeyCode::AnyKey))
			mInputArmed = true;

		return;
	}

	if (Input::GetActionTap("menu_confirm"))
		GameRules::StartNewGame();
	else if (Input::GetActionTap("menu_back"))
		SceneManager::LoadScene("Scenes/MainMenu.lnscene");
}

LION_REGISTER_COMPONENT(EndScreen)
