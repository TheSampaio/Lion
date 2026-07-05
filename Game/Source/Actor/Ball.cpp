#include "Ball.h"
#include "Paddle.h"

using namespace Lion;

Ball::Ball(Reference<Paddle> paddle)
	: mPaddle(std::move(paddle))
{
}

void Ball::OnAwake()
{
	// The depth is kept by the physics sync, so set it once here.
	GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Middle));

	mRenderer = AddComponent<SpriteRenderer>("Resource/Sprite/Brickout/ball.png");

	// Dynamic, frictionless, perfectly elastic ball with a fixed rotation and a circular shape.
	mBody = AddComponent<RigidBody2D>(BodyType::Dynamic, true);
	AddComponent<CircleCollider2D>(mRenderer->GetSize().width * 0.5f, 1.0f, 0.0f, 1.0f);

	const float32 ballRadius = mRenderer->GetSize().height * 0.5f;
	const float32 paddleHalfHeight = mPaddle->GetComponent<SpriteRenderer>()->GetSize().height * 0.5f;
	mAttachOffsetY = paddleHalfHeight + ballRadius + kAttachGap;

	Reset();
}

void Ball::OnUpdate()
{
	if (mState == State::Attached)
	{
		FollowPaddle();

		// Launch the ball with Space or Enter.
		if (Input::GetKeyPress(KeyCode::Space) || Input::GetKeyPress(KeyCode::Return))
		{
			mState = State::Launched;
			mBody->SetLinearVelocity(LaunchVelocity());
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

void Ball::OnCollision(Actor& other)
{
	// No steering while the ball is resting on the paddle.
	if (mState != State::Launched)
		return;

	// Steering only happens on the paddle; walls and bricks bounce through the physics solver.
	Paddle* paddle = dynamic_cast<Paddle*>(&other);

	if (!paddle)
		return;

	const float32 ballX = GetTransform()->GetPosition().x;
	const float32 paddleX = paddle->GetTransform()->GetPosition().x;
	const float32 paddleHalfWidth = paddle->GetComponent<SpriteRenderer>()->GetSize().width * 0.5f;

	// Offset of the hit from the paddle center, in [-1, 1]; the edges deflect the ball the most.
	const float32 offset = glm::clamp((ballX - paddleX) / paddleHalfWidth, -1.0f, 1.0f);
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

glm::vec2 Ball::LaunchVelocity() const
{
	// Launch mostly upward with a slight rightward lean, at the constant travel speed.
	return glm::normalize(glm::vec2(1.0f, 2.0f)) * kSpeed;
}

void Ball::FollowPaddle()
{
	// Sit just above the paddle and move with it while attached.
	const Vector paddlePosition = mPaddle->GetTransform()->GetPosition();

	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	mBody->SetPosition(glm::vec2(paddlePosition.x, paddlePosition.y + mAttachOffsetY));
}
