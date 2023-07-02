#pragma once

#include <Owl.h>

#include "Spaceship.h"
#include "Alien.h"

class Sandbox : public owl::Game
{
public:
	Sandbox();

	void OnStart();
	void OnUpdate();
	void OnDraw();
	void OnFinish();

	static owl::Scene* s_Scene;

private:
	owl::Sprite* m_Background;
	owl::Sprite* m_Header;

	Spaceship* m_Player;
	Alien* m_Alien;

	owl::Texture* m_Alien01;
	owl::Texture* m_Alien02;
	owl::Texture* m_Alien03;
	owl::Texture* m_Alien04;
};

