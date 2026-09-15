#include "Ball.h"
#include "Brick.h"
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
	FindAudioPlayers();

	Reset();
}

void Ball::OnUpdate()
{
	if (mState == State::Attached)
	{
		FollowPaddle();

		// Launch through the project action so keyboard and gamepad remain interchangeable.
		if (Input::GetActionTap("player_launch"))
			Launch(glm::vec2(0.0f, 1.0f));

		return;
	}

	glm::vec2 velocity = mBody->GetLinearVelocity();

	// Ignore a stopped ball (game over): do not relaunch it.
	if (velocity.x * velocity.x + velocity.y * velocity.y < 1.0f)
		return;

	// Keep a minimum vertical component so the ball never gets stuck bouncing horizontally.
	const float32 minVerticalSpeed = mSpeed * mMinVerticalRatio;

	if (std::abs(velocity.y) < minVerticalSpeed)
		velocity.y = (velocity.y >= 0.0f) ? minVerticalSpeed : -minVerticalSpeed;

	// Hold a constant speed so collisions only change direction, never energy (no runaway ball).
	mBody->SetLinearVelocity(glm::normalize(velocity) * mSpeed);
}

void Ball::OnCollision(Entity& other)
{
	// No steering while the ball is resting on the paddle.
	if (mState != State::Launched)
		return;

	if (!mImpactGeneral || !mImpactPoint)
		FindAudioPlayers();

	AudioPlayer* impact = other.HasComponent<Brick>() ? mImpactPoint : mImpactGeneral;

	if (impact)
		impact->Play();

	// Steering only happens on the paddle; walls and bricks bounce through the physics solver.
	Paddle* paddle = other.GetComponent<Paddle>();

	if (!paddle)
		return;

	const float32 ballX = GetOwner().GetWorldPosition().x;
	const float32 paddleX = paddle->GetOwner().GetWorldPosition().x;

	// A broad center lane returns the ball vertically. The outer lanes progressively steer left or
	// right, while the paddle's current movement adds a small, deliberate nudge. This keeps aiming
	// predictable without demanding pixel-perfect contact.
	const float32 offset = glm::clamp((ballX - paddleX) / paddle->GetHalfWidth(), -1.0f, 1.0f);
	const float32 absoluteOffset = std::abs(offset);
	float32 steering = absoluteOffset <= 0.22f
		? 0.0f
		: std::copysign((absoluteOffset - 0.22f) / 0.78f, offset);
	steering = glm::clamp(steering + paddle->GetMoveDirection() * 0.24f, -1.0f, 1.0f);
	if (std::abs(steering) < 0.12f)
		steering = 0.0f;
	const float32 angle = glm::radians(steering * mMaxBounceDegrees);

	// Always send the ball upward, angled by where it landed on the paddle.
	mBody->SetLinearVelocity(glm::vec2(mSpeed * std::sin(angle), mSpeed * std::cos(angle)));
}

void Ball::Reflect(Reflector& reflector)
{
	reflector.Field("Speed", mSpeed);
	reflector.Field("Minimum Vertical Ratio", mMinVerticalRatio);
	reflector.Field("Maximum Bounce Angle", mMaxBounceDegrees);
	reflector.Field("Attach Gap", mAttachGap);
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
	mBody->SetLinearVelocity(glm::normalize(direction) * mSpeed);
}

void Ball::LaunchFrom(const Vector2& position, const glm::vec2& direction)
{
	if (!mBody)
		return;

	GetOwner().SetWorldPosition(position);
	mBody->SetPosition(glm::vec2(position.x, position.y));
	SetVisible(true);
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

void Ball::FindAudioPlayers()
{
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> general = scene ? scene->FindEntity("Impact General Audio") : nullptr;
	const Reference<Entity> point = scene ? scene->FindEntity("Impact Point Audio") : nullptr;
	mImpactGeneral = general ? general->GetComponent<AudioPlayer>() : nullptr;
	mImpactPoint = point ? point->GetComponent<AudioPlayer>() : nullptr;
}

LION_REGISTER_COMPONENT(Ball)
