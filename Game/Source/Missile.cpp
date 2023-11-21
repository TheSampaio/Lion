#include "Missile.h"
#include "../Sandbox.h"

using namespace Lion;

Missile::Missile(Texture* Missile)
{
	m_Sprite = new Sprite(Missile);
	m_Speed = 400.0f;
}

Missile::~Missile()
{
	delete m_Sprite;
}

void Missile::OnDraw()
{
	m_Sprite->Draw(X, Y, Z);
}

void Missile::OnUpdate()
{
	AddMovement(0.0f, -m_Speed * Time::GetDeltaTime(), 0.0f);

	if (Y < 0)
		Sandbox::s_Scene->Remove();
}
