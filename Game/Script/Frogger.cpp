#include "Frogger.h"

using namespace owl;

enum State
{
	Up = 0,
	Down,
	Right,
	Left
};

Frogger::Frogger()
{
	m_SpriteUp = new Sprite("Data/Frogger/frog-up.png");
	m_SpriteDown = new Sprite("Data/Frogger/frog-down.png");
	m_SpriteRight = new Sprite("Data/Frogger/frog-right.png");
	m_SpriteLeft = new Sprite("Data/Frogger/frog-left.png");

	SetPosition(Window::GetCenter()[0] - m_SpriteUp->GetSize()[0] / 2.0f, Window::GetSize()[1] - 55.0f, Layer::Middle);

	m_State = Up;
	m_Speed = 40.0f;
}

Frogger::~Frogger()
{
	delete m_SpriteUp;
	delete m_SpriteDown;
	delete m_SpriteRight;
	delete m_SpriteLeft;
}

void Frogger::OnUpdate()
{
	if (Input::GetKeyTap(W) || Input::GetKeyTap(ArrowUp))
	{
		AddMovement(0.0f, -m_Speed, 0.0f);
		m_State = Up;
	}

	else if (Input::GetKeyTap(S) || Input::GetKeyTap(ArrowDown))
	{
		AddMovement(0.0f, m_Speed, 0.0f);
		m_State = Down;
	}

	if (Input::GetKeyTap(D) || Input::GetKeyTap(ArrowRight))
	{
		AddMovement(m_Speed, 0.0f, 0.0f);
		m_State = Right;
	}

	else if (Input::GetKeyTap(A) || Input::GetKeyTap(ArrowLeft))
	{
		AddMovement(-m_Speed, 0.0f, 0.0f);
		m_State = Left;
	}

	// Window's space controller

	if (X < 0)
		SetPosition(0.0f, Y, Z);

	else if (X > Window::GetSize()[0] - m_SpriteUp->GetSize()[0])
		SetPosition(static_cast<float>(Window::GetSize()[0] - m_SpriteUp->GetSize()[0]), Y, Z);

	if (Y < 65)
		SetPosition(X, 65.0f, Z);

	else if (Y > 545)
		SetPosition(X, 545.0f, Z);
}

void Frogger::OnDraw()
{
	if (m_State == Up)
		m_SpriteUp->Draw(X, Y, Z);

	else if (m_State == Down)
		m_SpriteDown->Draw(X, Y, Z);

	else if (m_State == Right)
		m_SpriteRight->Draw(X, Y, Z);

	else
		m_SpriteLeft->Draw(X, Y, Z);
}
