#include "Spaceship.h"
#include "Missile.h"

#include "Sandbox.h"

using namespace Lion;

Spaceship::Spaceship()
{
	m_Sprite = new Sprite("Data/Galaga/spaceship.png");
	m_Missile = new Texture("Data/Galaga/missile.png");
	m_Speed = 400.0f;

	SetPosition(static_cast<float>(Window::GetCenter()[0] - m_Sprite->GetCenter()[0]), static_cast<float>(Window::GetSize()[1] - 60), Layer::Upper);
}

Spaceship::~Spaceship()
{
	delete m_Sprite;
	delete m_Missile;
}

void Spaceship::OnDraw()
{
	m_Sprite->Draw(X, Y, Z);
}

void Spaceship::OnUpdate()
{
	if (Input::GetKeyPress(Key_D) || Input::GetKeyPress(Key_Right))
		AddMovement(static_cast<float>(m_Speed * Time::GetDeltaTime()), 0.0f, 0.0f);

	else if (Input::GetKeyPress(Key_A) || Input::GetKeyPress(Key_Left))
		AddMovement(static_cast<float>(-m_Speed * Time::GetDeltaTime()), 0.0f, 0.0f);

	if (Input::GetKeyTap(Key_Space))
	{
		Missile* Shoot = new Missile(m_Missile);
		Shoot->SetPosition(X + m_Sprite->GetCenter()[0] - m_Missile->GetCenter()[0], Y, Layer::Middle);

		Sandbox::s_Scene->Add(Shoot);
	}

	// Screen's space treatment
	if (X > Window::GetSize()[0] - m_Sprite->GetSize()[0])
		SetPosition(static_cast<float>(Window::GetSize()[0] - m_Sprite->GetSize()[0]), Y, Z);

	else if (X < 0)
		SetPosition(0, Y, Z);
}
