#include "Ball.h"
#include "Brick.h"
#include "GameAudio.h"
#include "Paddle.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void Ball::OnAwake()
{
	mBody = GetOwner().GetComponent<RigidBody2D>();
	mRenderer = GetOwner().GetComponent<SpriteRenderer>();
	mPaddle = GetOwner().GetScene()->FindComponent<Paddle>();

	if (!mBody || !mRenderer || !mPaddle)
	{
		Log::Console(LogLevel::Error,
			"[Ball] Requires a RigidBody2D and SpriteRenderer, plus a Paddle in the same scene.");
		SetEnabled(false);
		return;
	}

	const float32 ballRadius = mRenderer->GetSize().height
		* GetOwner().GetWorldScale().y * 0.5f;
	mAttachOffsetY = mPaddle->GetHalfHeight() + ballRadius + mAttachGap;
	Reset();
}

void Ball::OnUpdate()
{
	if (mState == State::Attached)
	{
		FollowPaddle();

		// Launch through the project action so keyboard and gamepad remain interchangeable.
		if (Input::GetActionTap("player_launch"))
			Launch(glm::vec2(0.045f * mLastHorizontalSign, 1.0f));

		return;
	}

	glm::vec2 velocity = mBody->GetLinearVelocity();

	// Ignore a stopped ball (game over): do not relaunch it.
	if (velocity.x * velocity.x + velocity.y * velocity.y < 1.0f)
		return;

	// The first shot is deliberately predictable and nearly vertical. Once it touches the arena, the
	// anti-lock angle constraints take over for the remainder of that ball's life.
	mBody->SetLinearVelocity((mHasBounced ? CorrectDirection(velocity) : glm::normalize(velocity)) * mSpeed);
}

void Ball::OnCollision(Entity& other)
{
	// No steering while the ball is resting on the paddle.
	if (mState != State::Launched)
		return;

	if (other.HasComponent<Brick>()) GameAudio::PlaySfx("Sounds/ball-brick.wav", 0.52f);
	else if (other.HasComponent<Paddle>()) GameAudio::PlaySfx("Sounds/ball-paddle.wav", 0.62f);
	else if (other.GetName().find("Arena") != std::string::npos) GameAudio::PlaySfx("Sounds/ball-bumper.wav", 0.58f);
	else GameAudio::PlaySfx("Sounds/ball-wall.wav", 0.36f);
	mHasBounced = true;

	// Steering only happens on the paddle; walls and bricks bounce through the physics solver.
	Paddle* paddle = other.GetComponent<Paddle>();

	if (!paddle)
		return;

	const float32 ballX = GetOwner().GetWorldPosition().x;
	const float32 paddleX = paddle->GetOwner().GetWorldPosition().x;

	// Map the hit position to a controlled angle and add paddle movement. The center still aims nearly
	// straight, but never perfectly vertical, so one safe lane cannot repeat forever.
	const float32 offset = glm::clamp((ballX - paddleX) / paddle->GetHalfWidth(), -1.0f, 1.0f);
	float32 steering = offset;
	steering = glm::clamp(steering + paddle->GetMoveDirection() * 0.24f, -1.0f, 1.0f);
	if (std::abs(steering) < mMinHorizontalRatio)
		steering = std::copysign(mMinHorizontalRatio,
			std::abs(paddle->GetMoveDirection()) > 0.05f ? paddle->GetMoveDirection() : mLastHorizontalSign);
	const float32 angle = glm::radians(steering * mMaxBounceDegrees);

	// Always send the ball upward, angled by where it landed on the paddle.
	mBody->SetLinearVelocity(glm::vec2(mSpeed * std::sin(angle), mSpeed * std::cos(angle)));
}

void Ball::Reflect(Reflector& reflector)
{
	reflector.Field("Speed", mSpeed);
	reflector.Field("Minimum Vertical Ratio", mMinVerticalRatio);
	reflector.Field("Minimum Horizontal Ratio", mMinHorizontalRatio);
	reflector.Field("Maximum Bounce Angle", mMaxBounceDegrees);
	reflector.Field("Attach Gap", mAttachGap);
}

void Ball::Reset()
{
	mState = State::Attached;
	mLastHorizontalSign = -mLastHorizontalSign;
	mHasBounced = false;
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

void Ball::SetSpeed(float32 speed)
{
	mSpeed = std::max(speed, 1.0f);

	if (mState != State::Launched)
		return;

	const glm::vec2 velocity = mBody->GetLinearVelocity();

	if (glm::dot(velocity, velocity) > 0.0f)
		mBody->SetLinearVelocity(glm::normalize(velocity) * mSpeed);
}

void Ball::Launch(const glm::vec2& direction)
{
	if (!mBody || glm::dot(direction, direction) <= 0.0f)
		return;

	mState = State::Launched;
	const glm::vec2 launchDirection = mHasBounced ? CorrectDirection(direction) : glm::normalize(direction);
	mBody->SetLinearVelocity(launchDirection * mSpeed);
}

void Ball::LaunchFrom(const Vector2& position, const glm::vec2& direction)
{
	if (!mBody)
		return;

	GetOwner().SetWorldPosition(position);
	mBody->SetPosition(glm::vec2(position.x, position.y));
	SetVisible(true);
	mHasBounced = true;
	Launch(direction);
}

glm::vec2 Ball::GetDirection() const
{
	if (!mBody)
		return glm::vec2(0.0f, 1.0f);

	const glm::vec2 velocity = mBody->GetLinearVelocity();
	return glm::dot(velocity, velocity) > 0.0f
		? glm::normalize(velocity)
		: glm::vec2(0.0f, 1.0f);
}

void Ball::FollowPaddle()
{
	// Sit just above the paddle and move with it while attached.
	const Vector paddlePosition = mPaddle->GetOwner().GetWorldPosition();

	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	mBody->SetPosition(glm::vec2(paddlePosition.x, paddlePosition.y + mAttachOffsetY));
}

glm::vec2 Ball::CorrectDirection(const glm::vec2& direction)
{
	if (glm::dot(direction, direction) <= 0.0f)
		return glm::vec2(mMinHorizontalRatio * mLastHorizontalSign, 1.0f);

	glm::vec2 corrected = glm::normalize(direction);
	if (std::abs(corrected.x) < mMinHorizontalRatio)
		corrected.x = std::copysign(mMinHorizontalRatio,
			std::abs(corrected.x) > 0.001f ? corrected.x : mLastHorizontalSign);
	if (std::abs(corrected.y) < mMinVerticalRatio)
		corrected.y = std::copysign(mMinVerticalRatio,
			std::abs(corrected.y) > 0.001f ? corrected.y : 1.0f);

	corrected = glm::normalize(corrected);
	mLastHorizontalSign = corrected.x >= 0.0f ? 1.0f : -1.0f;
	return corrected;
}

LION_REGISTER_COMPONENT(Ball)
