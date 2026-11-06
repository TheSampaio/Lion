#pragma once

#include <Lion/Lion.h>

class Paddle;

// The ball: rests on the paddle until launched, then travels at a constant speed and takes its
// direction from wherever it lands on the paddle.
class Ball : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void OnCollision(Lion::Entity& other) override;
	void Reflect(Lion::Reflector& reflector) override;

	// Re-attaches the ball to the paddle, ready to be launched again.
	void Reset();

	// Freezes the ball in place, and shows or hides it (both used on game over).
	void Stop();
	void SetVisible(bool visible);
	void SetSpeed(Lion::float32 speed);
	void Launch(const glm::vec2& direction);
	void LaunchFrom(const Lion::Vector2& position, const glm::vec2& direction);
	void SetPiercing(bool piercing);
	bool IsLaunched() const { return mState == State::Launched; }
	bool IsPiercing() const { return mPiercing; }
	glm::vec2 GetDirection() const;

private:
	enum class State
	{
		Attached,  // Resting on the paddle, waiting to be launched.
		Launched,  // In play.
	};

	Lion::float32 mSpeed = 390.0f;            // Constant travel speed (pixels/s).
	Lion::float32 mMinVerticalRatio = 0.35f;  // Keeps the ball from travelling nearly horizontally.
	Lion::float32 mMinHorizontalRatio = 0.20f; // Keeps the ball from repeating a vertical lane forever.
	Lion::float32 mMaxBounceDegrees = 55.0f;  // Paddle steering range from vertical.
	Lion::float32 mAttachGap = 2.0f;          // Small gap above the paddle.

	State mState = State::Attached;
	Paddle* mPaddle = nullptr;
	Lion::RigidBody2D* mBody = nullptr;
	Lion::SpriteRenderer* mRenderer = nullptr;
	Lion::CircleCollider2D* mCollider = nullptr;
	Lion::Vector2 mBaseScale{ 1.0f, 1.0f };
	Lion::float32 mAttachOffsetY = 0.0f;
	Lion::float32 mLastHorizontalSign = 1.0f;
	glm::vec2 mIncomingDirection{ 0.0f, 1.0f };
	bool mHasBounced = false;
	bool mPiercing = false;

	void FollowPaddle();
	void UpdatePiercingScale();
	glm::vec2 CorrectDirection(const glm::vec2& direction);
};
