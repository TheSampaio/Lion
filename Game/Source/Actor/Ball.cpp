#include "Ball.h"

using namespace Lion;

void Ball::OnAwake()
{
	GetTransform()->SetPosition(Vector(kStartX, kStartY, Depth::Middle));

	SpriteRenderer* renderer = AddComponent<SpriteRenderer>("Resource/Sprite/Brickout/ball.png");

	// Dynamic, frictionless, perfectly elastic ball with a fixed rotation and a circular shape.
	mBody = AddComponent<RigidBody2D>(BodyType::Dynamic, true);
	AddComponent<CircleCollider2D>(renderer->GetSize().width * 0.5f, 1.0f, 0.0f, 1.0f);

	mBody->SetLinearVelocity(glm::vec2(kSpeedX, kSpeedY));
}

void Ball::Reset()
{
	GetTransform()->SetPosition(Vector(kStartX, kStartY, Depth::Middle));

	mBody->SetPosition(glm::vec2(kStartX, kStartY));
	mBody->SetLinearVelocity(glm::vec2(kSpeedX, kSpeedY));
}

void Ball::Stop()
{
	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
}
