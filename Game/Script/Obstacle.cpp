#include "Obstacle.h"

using namespace owl;

Obstacle::Obstacle(owl::Texture* Texture, float Speed, float Z)
{
	m_Sprite = new Sprite(Texture);
	m_Speed = Speed;
	this->Z = Z;
}

Obstacle::~Obstacle()
{
	delete m_Sprite;
}

void Obstacle::OnUpdate()
{
	AddMovement(-m_Speed * Time::GetDeltaTime(), 0.0f, 0.0f);

	if (X + m_Sprite->GetSize()[0] < 0)
		SetPosition(static_cast<float>(Window::GetSize()[0] + m_Sprite->GetSize()[0]), Y, Z);
}
