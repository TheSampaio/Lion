#pragma once

#include <Lion.h>

class Spaceship : public Lion::Entity
{
public:
	Spaceship();
	~Spaceship();

	void OnDraw();
	void OnUpdate();

	Lion::Sprite* GetSprite() const { return m_Sprite; }

private:
	Lion::Sprite* m_Sprite;
	Lion::Texture* m_Missile;

	float m_Speed;
};

