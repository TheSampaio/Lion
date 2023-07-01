#pragma once

#include <Owl.h>

class Sandbox : public owl::Game
{
public:
	Sandbox();

	void OnStart();
	void OnUpdate();
	void OnDraw();
	void OnFinish();

private:
	std::list<owl::Entity*> m_Scene;

	owl::Texture* m_Truck;
	owl::Texture* m_Car01;
	owl::Texture* m_Car02;
	owl::Texture* m_Car03;
	owl::Texture* m_Car04;
	owl::Texture* m_Turtle01;
	owl::Texture* m_Turtle02;
	owl::Texture* m_Wood01;
	owl::Texture* m_Wood02;

	owl::Sprite* m_Background;

	class Frogger* m_Player;
	class Obstacle* m_Obstacle;
};

