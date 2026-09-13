#include "MainMenu.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void MainMenu::OnUpdate()
{
	if (Input::GetKeyTap(KeyCode::AnyKey) && !mStartScene.empty())
		SceneManager::LoadScene(mStartScene);
}

void MainMenu::Reflect(Reflector& reflector)
{
	reflector.FieldAsset("Start Scene", mStartScene);
}

LION_REGISTER_COMPONENT(MainMenu)
