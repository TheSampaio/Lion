#pragma once

#include <Lion/Lion.h>

// Drives the player's paddle: moves it with the arrow keys and keeps it inside the play area.
//
// It expects its entity to carry a kinematic RigidBody2D and a SpriteRenderer — a component asks its
// owner for the traits it needs rather than being those traits itself.
class Paddle : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void Reflect(Lion::Reflector& reflector) override;

	// Returns the paddle to the position authored in the scene.
	void Reset();

	// The paddle's half extents: the ball bounces off its width and rests on top of its height.
	Lion::float32 GetHalfWidth() const;
	Lion::float32 GetHalfHeight() const;

private:
	Lion::float32 mSpeed = 500.0f;
	Lion::float32 mHorizontalLimit = 350.0f;

	Lion::RigidBody2D* mBody = nullptr;
	Lion::SpriteRenderer* mRenderer = nullptr;
	Lion::Vector2 mStartPosition;
};
