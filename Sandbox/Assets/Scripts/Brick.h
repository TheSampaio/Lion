#pragma once

#include <Lion/Lion.h>

class Ball;

// An authored brick whose remaining durability is communicated by a single shared colour ladder.
class Brick : public Lion::Component
{
public:
	void OnAwake() override;
	void OnCollision(Lion::Entity& other) override;
	void Reflect(Lion::Reflector& reflector) override;
	void Damage(Lion::int32 amount, Ball* sourceBall, bool triggerPower = true);

private:
	Lion::int32 mHitPoints = 1;
	Lion::int32 mRemainingHits = 1;
	std::string mPower;
	Lion::SpriteRenderer* mRenderer = nullptr;

	void UpdateAppearance();
};
