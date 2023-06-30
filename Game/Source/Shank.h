#pragma once

#include <Owl.h>

class Shank : public owl::Entity
{
public:
	Shank();
	~Shank();

	void OnUpdate();
	void OnDraw();

private:
	owl::Sprite* m_Sprite;
	float m_Speed;
};

