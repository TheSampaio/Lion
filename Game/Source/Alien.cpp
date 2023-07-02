#include "Alien.h"

using namespace owl;

Alien::Alien(owl::Texture* Alien, float X, float Y)
{
	m_Sprite = new Sprite(Alien);
	m_Speed = 150.0f;

	SetPosition(X, Y, Layer::Lower);
}

Alien::~Alien()
{
	delete m_Sprite;
}

void Alien::OnDraw()
{
	m_Sprite->Draw(X, Y, Z);
}

void Alien::OnUpdate()
{
	AddMovement(m_Speed * Time::GetDeltaTime(), 0.0f, 0.0f);

	if (X > Window::GetSize()[0])
		SetPosition(static_cast<float>(-static_cast<int>(m_Sprite->GetSize()[0])), Y, Z);
}
