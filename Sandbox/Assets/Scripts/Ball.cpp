#include "Ball.h"
#include "Paddle.h"
#include "SceneQuery.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void Ball::OnAwake()
{
	mBody = GetOwner().GetComponent<RigidBody2D>();
	mRenderer = GetOwner().GetComponent<SpriteRenderer>();
	mPaddle = FindInScene<Paddle>(GetOwner().GetScene());

	const float32 ballRadius = mRenderer->GetSize().height * 0.5f;
	mAttachOffsetY = mPaddle->GetHalfHeight() + ballRadius + kAttachGap;

	Reset();
}

void Ball::OnUpdate()
{
	if (mState == State::Attached)
	{
		FollowPaddle();

		// Launch the ball with Space or Enter, mostly upward with a slight rightward lean.
		if (Input::GetKeyPress(KeyCode::Space) || Input::GetKeyPress(KeyCode::Return))
		{
			mState = State::Launched;
			mBody->SetLinearVelocity(glm::normalize(glm::vec2(1.0f, 2.0f)) * kSpeed);
		}

		return;
	}

	glm::vec2 velocity = mBody->GetLinearVelocity();

	// Ignore a stopped ball (game over): do not relaunch it.
	if (velocity.x * velocity.x + velocity.y * velocity.y < 1.0f)
		return;

	// Keep a minimum vertical component so the ball never gets stuck bouncing horizontally.
	const float32 minVerticalSpeed = kSpeed * kMinVerticalRatio;

	if (std::abs(velocity.y) < minVerticalSpeed)
		velocity.y = (velocity.y >= 0.0f) ? minVerticalSpeed : -minVerticalSpeed;

	// Hold a constant speed so collisions only change direction, never energy (no runaway ball).
	mBody->SetLinearVelocity(glm::normalize(velocity) * kSpeed);
}

void Ball::OnCollision(Entity& other)
{
	// No steering while the ball is resting on the paddle.
	if (mState != State::Launched)
		return;

	// Steering only happens on the paddle; walls and bricks bounce through the physics solver.
	Paddle* paddle = other.GetComponent<Paddle>();

	if (!paddle)
		return;

	const float32 ballX = GetTransform()->GetPosition().x;
	const float32 paddleX = paddle->GetTransform()->GetPosition().x;

	// Offset of the hit from the paddle's center, in [-1, 1]; the edges deflect the ball the most.
	const float32 offset = glm::clamp((ballX - paddleX) / paddle->GetHalfWidth(), -1.0f, 1.0f);
	const float32 angle = glm::radians(offset * kMaxBounceDegrees);

	// Always send the ball upward, angled by where it landed on the paddle.
	mBody->SetLinearVelocity(glm::vec2(kSpeed * std::sin(angle), kSpeed * std::cos(angle)));
}

void Ball::Reset()
{
	mState = State::Attached;
	FollowPaddle();
	SetVisible(true);
}

void Ball::Stop()
{
	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
}

void Ball::SetVisible(bool visible)
{
	mRenderer->SetEnabled(visible);
}

void Ball::FollowPaddle()
{
	// Sit just above the paddle and move with it while attached.
	const Vector paddlePosition = mPaddle->GetTransform()->GetPosition();

	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	mBody->SetPosition(glm::vec2(paddlePosition.x, paddlePosition.y + mAttachOffsetY));
}

LION_REGISTER_COMPONENT(Ball)
