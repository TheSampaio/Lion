#include "Brick.h"
#include "Ball.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void Brick::OnAwake()
{
	mHitPoints = std::clamp(mHitPoints, 1, 5);
	mRemainingHits = mHitPoints;
	mRenderer = GetOwner().GetComponent<SpriteRenderer>();
	UpdateAppearance();
}

void Brick::OnCollision(Entity& other)
{
	if (!GetOwner().IsEnabled() || !other.HasComponent<Ball>())
		return;

	Ball* ball = other.GetComponent<Ball>();
	Damage(ball && ball->IsPiercing() ? 99 : 1, ball);
}

void Brick::Damage(int32 amount, Ball* sourceBall, bool triggerPower)
{
	if (!GetOwner().IsEnabled() || mRemainingHits <= 0 || amount <= 0)
		return;

	mRemainingHits = std::max(mRemainingHits - amount, 0);
	const bool destroyed = mRemainingHits == 0;
	GameRules::RegisterBrickDamage(GetOwner().GetWorldPosition(), destroyed,
		triggerPower ? mPower : std::string(), sourceBall);

	if (!destroyed)
	{
		UpdateAppearance();
		return;
	}

	GetOwner().SetVisible(false);
	GetOwner().SetEnabled(false);
}

void Brick::Reflect(Reflector& reflector)
{
	reflector.Field("Hit Points", mHitPoints);
	reflector.Field("Power", mPower);
}

void Brick::UpdateAppearance()
{
	if (!mRenderer)
		return;

	const int32 tier = std::clamp(mRemainingHits, 1, 5);
	mRenderer->SetTexturePath(LION_FORMAT_TEXT("Sprites/Brickout/tile-{}.png", tier));
}

LION_REGISTER_COMPONENT(Brick)
