#pragma once

#include <Owl.h>

class Spaceship : public owl::Entity
{
public:
	Spaceship();
	~Spaceship();

	void OnDraw();
	void OnUpdate();

	owl::Sprite* GetSprite() const { return m_Sprite; }

private:
	owl::Sprite* m_Sprite;
	owl::Texture* m_Missile;

	float m_Speed;
};

