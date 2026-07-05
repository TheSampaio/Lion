#pragma once

#include <Lion/Lion.h>

class Ball : public Lion::Actor
{
public:
	void OnAwake() override;

	// Restores the ball to its starting position and launch velocity.
	void Reset();

	// Freezes the ball in place (used on game over).
	void Stop();

private:
	static constexpr Lion::float32 kStartX = 0.0f;
	static constexpr Lion::float32 kStartY = -100.0f;
	static constexpr Lion::float32 kSpeedX = 250.0f;
	static constexpr Lion::float32 kSpeedY = 350.0f;

	Lion::RigidBody2D* mBody = nullptr;
};
