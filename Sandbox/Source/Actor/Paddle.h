#pragma once

#include <Lion/Lion.h>

class Paddle : public Lion::Actor
{
public:
	void OnAwake() override;
	void OnUpdate() override;

private:
	Lion::RigidBody2D* mBody = nullptr;
	Lion::SpriteRenderer* mRenderer = nullptr;

	const Lion::float32 mSpeed = 500.0f;
};
