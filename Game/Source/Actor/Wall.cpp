#include "Wall.h"

using namespace Lion;

Wall::Wall(Vector position, float32 width, float32 height)
	: mPosition(position), mWidth(width), mHeight(height)
{
}

void Wall::OnAwake()
{
	GetTransform()->SetPosition(mPosition);

	// Static body with a fully elastic collider so the ball bounces without losing energy.
	AddComponent<RigidBody2D>(BodyType::Static);
	AddComponent<BoxCollider2D>(mWidth, mHeight, 1.0f, 0.0f, 1.0f);
}
