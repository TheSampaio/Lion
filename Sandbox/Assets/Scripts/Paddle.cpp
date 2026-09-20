#include "Paddle.h"

#include <Lion/Core/Clock.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void Paddle::OnAwake()
{
	mBody = GetOwner().GetComponent<RigidBody2D>();
	mRenderer = GetOwner().GetComponent<SpriteRenderer>();
	mCollider = GetOwner().GetComponent<BoxCollider2D>();
	mStartPosition = GetOwner().GetWorldPosition();
	mBaseScale = GetOwner().GetWorldScale();

	if (!mBody || !mRenderer || !mCollider)
	{
		Log::Console(LogLevel::Error,
			"[Paddle] Requires a RigidBody2D, BoxCollider2D and SpriteRenderer on the same entity.");
		SetEnabled(false);
		return;
	}

	mBaseHalfWidth = GetHalfWidth();
}

void Paddle::OnUpdate()
{
	const float32 extraHalfWidth = std::max(GetHalfWidth() - mBaseHalfWidth, 0.0f);
	const float32 limit = std::max(mHorizontalLimit - extraHalfWidth, 0.0f);
	if (mFollowTarget)
	{
		mMoveDirection = mFollowTarget->GetMoveDirection();
		const Vector2 target = mFollowTarget->GetOwner().GetWorldPosition();
		float32 requestedX = target.x + mFollowOffset;
		if (requestedX < -limit || requestedX > limit)
			requestedX = target.x - mFollowOffset;
		const float32 x = std::clamp(requestedX, -limit, limit);
		mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
		mBody->SetPosition(glm::vec2(x, target.y));
		return;
	}

	mMoveDirection = Input::GetActionStrength("player_right")
		- Input::GetActionStrength("player_left");
	Vector2 position = GetOwner().GetWorldPosition();
	const float32 clampedX = std::clamp(position.x, -limit, limit);
	if (clampedX != position.x)
	{
		position.x = clampedX;
		mBody->SetPosition(glm::vec2(position.x, position.y));
	}

	if ((position.x >= limit && mMoveDirection > 0.0f)
		|| (position.x <= -limit && mMoveDirection < 0.0f))
	{
		mMoveDirection = 0.0f;
		mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
		return;
	}

	const float32 deltaTime = std::max(Clock::GetDeltaTime(), 0.0001f);
	const float32 requestedVelocity = mMoveDirection * mSpeed;
	const float32 destination = std::clamp(position.x + requestedVelocity * deltaTime, -limit, limit);
	mBody->SetLinearVelocity(glm::vec2((destination - position.x) / deltaTime, 0.0f));
}

void Paddle::Reflect(Reflector& reflector)
{
	reflector.Field("Speed", mSpeed);
	reflector.Field("Horizontal Limit", mHorizontalLimit);
}

void Paddle::Reset()
{
	SetWide(false);
	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	GetOwner().SetWorldPosition(mStartPosition);
	mBody->SetPosition(glm::vec2(mStartPosition.x, mStartPosition.y));
}

float32 Paddle::GetHalfWidth() const
{
	return mRenderer->GetSize().width * GetOwner().GetWorldScale().x * 0.5f;
}

float32 Paddle::GetHalfHeight() const
{
	return mRenderer->GetSize().height * GetOwner().GetWorldScale().y * 0.5f;
}

void Paddle::SetWide(bool wide)
{
	if (!mCollider || wide == mWide)
		return;

	mWide = wide;
	Vector2 scale = mBaseScale;
	if (wide)
		scale.x *= 1.55f;
	GetOwner().SetWorldScale(scale);
	mCollider->RefreshShape();
}

void Paddle::Follow(Paddle& target, float32 horizontalOffset)
{
	mFollowTarget = &target;
	mFollowOffset = horizontalOffset;
}

LION_REGISTER_COMPONENT(Paddle)
