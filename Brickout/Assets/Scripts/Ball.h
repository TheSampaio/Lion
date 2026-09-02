#pragma once

#include <Lion/Lion.h>

class Paddle;

namespace Lion
{
	class AudioPlayer;
}

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

private:
	enum class State
	{
		Attached,  // Resting on the paddle, waiting to be launched.
		Launched,  // In play.
	};

	Lion::float32 mSpeed = 430.0f;            // Constant travel speed (pixels/s).
	Lion::float32 mMinVerticalRatio = 0.35f;  // Keeps the ball from going flat.
	Lion::float32 mMaxBounceDegrees = 60.0f;  // Paddle steering range from vertical.
	Lion::float32 mAttachGap = 2.0f;          // Small gap above the paddle.

	State mState = State::Attached;
	Paddle* mPaddle = nullptr;
	Lion::RigidBody2D* mBody = nullptr;
	Lion::SpriteRenderer* mRenderer = nullptr;
	Lion::float32 mAttachOffsetY = 0.0f;
	Lion::AudioPlayer* mImpactGeneral = nullptr;
	Lion::AudioPlayer* mImpactPoint = nullptr;

	void FollowPaddle();
	void FindAudioPlayers();
};
