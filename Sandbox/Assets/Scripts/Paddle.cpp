#include "Paddle.h"

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
	}
}

void Paddle::OnUpdate()
{
	mMoveDirection = Input::GetActionStrength("player_right")
		- Input::GetActionStrength("player_left");
	const float32 velocityX = mMoveDirection * mSpeed;

	mBody->SetLinearVelocity(glm::vec2(velocityX, 0.0f));

	const Vector position = GetOwner().GetWorldPosition();

	if (std::abs(position.x) <= mHorizontalLimit)
		return;

	mBody->SetPosition(glm::vec2((position.x > 0.0f) ? mHorizontalLimit : -mHorizontalLimit, position.y));
	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
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

LION_REGISTER_COMPONENT(Paddle)
