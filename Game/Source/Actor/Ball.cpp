#include "Ball.h"

using namespace Lion;

void Ball::OnAwake()
{
	GetTransform()->SetPosition(Vector(0.0f, -100.0f, Depth::Middle));

	SpriteRenderer* renderer = AddComponent<SpriteRenderer>("Resource/Sprite/Brickout/ball.png");
	const Size size = renderer->GetSize();

	// Dynamic, frictionless, perfectly elastic ball with a fixed rotation.
	RigidBody2D* body = AddComponent<RigidBody2D>(BodyType::Dynamic, true);
	AddComponent<CircleCollider2D>(size.width * 0.5f, 1.0f, 0.0f, 1.0f);

	// Launch the ball (pixels per second). Applied once the body is created.
	body->SetLinearVelocity(glm::vec2(250.0f, 350.0f));
}
