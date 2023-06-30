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
	owl::Sprite* m_Background;

	class Shank* m_Player;
	class Header* m_Header01;
	class Header* m_Header02;
};

