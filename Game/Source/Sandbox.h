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
	owl::Texture* m_HeaderTexture;
	owl::Sprite* m_Header01;
	owl::Sprite* m_Header02;

	owl::Sprite* m_Background;
	owl::Sprite* m_Player;

	float m_Speed;
};

