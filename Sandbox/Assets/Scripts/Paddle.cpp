#include "Paddle.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void Paddle::OnAwake()
{
	mBody = GetOwner().GetComponent<RigidBody2D>();
	mRenderer = GetOwner().GetComponent<SpriteRenderer>();
	mStartPosition = GetOwner().GetWorldPosition();

	if (!mBody || !mRenderer)
	{
		Log::Console(LogLevel::Error, "[Paddle] Requires a RigidBody2D and SpriteRenderer on the same entity.");
		SetEnabled(false);
	}
}

void Paddle::OnUpdate()
{
	float32 velocityX = 0.0f;

	if (Input::GetKeyPress(KeyCode::D) || Input::GetKeyPress(KeyCode::Right))
		velocityX = mSpeed;

	else if (Input::GetKeyPress(KeyCode::A) || Input::GetKeyPress(KeyCode::Left))
		velocityX = -mSpeed;

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
	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	GetOwner().SetWorldPosition(mStartPosition);
	mBody->SetPosition(glm::vec2(mStartPosition.x, mStartPosition.y));
}

float32 Paddle::GetHalfWidth() const
{
	return mRenderer->GetSize().width * 0.5f;
}

float32 Paddle::GetHalfHeight() const
{
	return mRenderer->GetSize().height * 0.5f;
}

LION_REGISTER_COMPONENT(Paddle)
