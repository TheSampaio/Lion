#include "Brick.h"

using namespace Lion;

Brick::Brick(Reference<Texture> texture, Vector position)
	: mTexture(std::move(texture)), mPosition(position)
{
}

void Brick::OnAwake()
{
	GetTransform()->SetPosition(mPosition);

	SpriteRenderer* renderer = AddComponent<SpriteRenderer>(mTexture);
	const Size size = renderer->GetSize();

	AddComponent<RigidBody2D>(BodyType::Static);
	AddComponent<BoxCollider2D>(size.width, size.height, 1.0f, 0.0f, 1.0f);
}

void Brick::OnCollision(Actor& other)
{
	// Only the dynamic ball collides with static bricks, so any contact destroys this brick.
	RemoveFromScene();
}
