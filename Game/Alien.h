#pragma once

#include <Owl.h>

class Alien : public owl::Entity
{
public:
	Alien(owl::Texture* Alien, float X, float Y);
	~Alien();

	void OnDraw();
	void OnUpdate();

private:
	owl::Sprite* m_Sprite;
	float m_Speed;
};
