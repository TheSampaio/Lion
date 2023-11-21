#pragma once

#include <Owl.h>

class Alien : public Lion::Entity
{
public:
	Alien(Lion::Texture* Alien, float X, float Y);
	~Alien();

	void OnDraw();
	void OnUpdate();

private:
	Lion::Sprite* m_Sprite;
	float m_Speed;
};
