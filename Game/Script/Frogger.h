#pragma once

#include <Owl.h>

class Frogger : public owl::Entity
{
public:
	Frogger();
	virtual ~Frogger();

	void OnUpdate();
	void OnDraw();
private:
	owl::Sprite* m_SpriteUp;
	owl::Sprite* m_SpriteDown;
	owl::Sprite* m_SpriteRight;
	owl::Sprite* m_SpriteLeft;

	int m_State;
	float m_Speed;
};

