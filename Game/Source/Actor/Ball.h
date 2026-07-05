#pragma once

#include <Lion/Lion.h>

class Ball : public Lion::Actor
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void OnCollision(Lion::Actor& other) override;

	// Restores the ball to its starting position, launch velocity and visibility.
	void Reset();

	// Freezes the ball in place (used on game over).
	void Stop();

	// Shows or hides the ball's sprite.
	void SetVisible(bool visible);

private:
	static constexpr Lion::float32 kSpeed = 430.0f;          // Constant travel speed (pixels/s).
	static constexpr Lion::float32 kStartX = 0.0f;
	static constexpr Lion::float32 kStartY = -100.0f;
	static constexpr Lion::float32 kMinVerticalRatio = 0.35f; // Keeps the ball from going flat.
	static constexpr Lion::float32 kMaxBounceDegrees = 60.0f;  // Paddle steering range from vertical.

	Lion::RigidBody2D* mBody = nullptr;
	Lion::SpriteRenderer* mRenderer = nullptr;

	glm::vec2 LaunchVelocity() const;
};
