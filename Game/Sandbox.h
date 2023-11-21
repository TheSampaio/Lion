#pragma once

#include <Owl.h>

#include "Spaceship.h"
#include "Alien.h"

class Sandbox : public Lion::Game
{
public:
	Sandbox();

	void OnStart();
	void OnUpdate();
	void OnDraw();
	void OnFinish();

	static Lion::Scene* s_Scene;

private:
	Lion::Sprite* m_Background;
	Lion::Sprite* m_Header;

	Spaceship* m_Player;
	Alien* m_Alien;

	Lion::Texture* m_Alien01;
	Lion::Texture* m_Alien02;
	Lion::Texture* m_Alien03;
	Lion::Texture* m_Alien04;
};

