#pragma once

#include <Lion/Lion.h>

class Paddle;

class Ball : public Lion::Actor
{
public:
	explicit Ball(Lion::Reference<Paddle> paddle);

	void OnAwake() override;
	void OnUpdate() override;
	void OnCollision(Lion::Actor& other) override;

	// Re-attaches the ball to the paddle, ready to be launched again.
	void Reset();

	// Freezes the ball in place (used on game over).
	void Stop();

	// Shows or hides the ball's sprite.
	void SetVisible(bool visible);

private:
	enum class State
	{
		Attached,  // Resting on the paddle, waiting to be launched.
		Launched,  // In play.
	};

	static constexpr Lion::float32 kSpeed = 430.0f;           // Constant travel speed (pixels/s).
	static constexpr Lion::float32 kMinVerticalRatio = 0.35f;  // Keeps the ball from going flat.
	static constexpr Lion::float32 kMaxBounceDegrees = 60.0f;  // Paddle steering range from vertical.
	static constexpr Lion::float32 kAttachGap = 2.0f;          // Small gap above the paddle.

	State mState = State::Attached;
	Lion::Reference<Paddle> mPaddle;
	Lion::RigidBody2D* mBody = nullptr;
	Lion::SpriteRenderer* mRenderer = nullptr;
	Lion::float32 mAttachOffsetY = 0.0f;

	glm::vec2 LaunchVelocity() const;
	void FollowPaddle();
};
