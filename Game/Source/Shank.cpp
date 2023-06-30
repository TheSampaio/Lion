#include "Shank.h"

using namespace owl;

Shank::Shank()
{
	m_Sprite = new Sprite("Resource/Shank/Sprite/shank.png");
	m_Speed = 150.0f;

	SetPosition(10.0f, 85.0f);
}

Shank::~Shank()
{
	delete m_Sprite;
}

void Shank::OnUpdate()
{
	AddMovement(m_Speed * Time::GetDeltaTime(), 0.0f, 0.0f);

	if (X > Window::GetSize()[0])
		X = static_cast<float>(-static_cast<int>(m_Sprite->GetSize()[0]));
}

void Shank::OnDraw()
{
	m_Sprite->Draw(X, Y, Z);
}
