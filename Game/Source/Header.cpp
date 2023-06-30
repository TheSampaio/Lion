#include "Header.h"

using namespace owl;

Header::Header(owl::Texture* Texture)
{
	m_Sprite = new Sprite(Texture);
}

Header::~Header()
{
	delete m_Sprite;
}

void Header::OnUpdate()
{
}

void Header::OnDraw()
{
	m_Sprite->Draw(X, Y, Z);
}
