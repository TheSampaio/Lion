#pragma once

#include <Owl.h>

class Header : public owl::Entity
{
public:
	Header(owl::Texture* Texture);
	~Header();

	void OnUpdate();
	void OnDraw();

private:
	owl::Sprite* m_Sprite;
};

