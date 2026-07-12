#pragma once

#include <Lion/Lion.h>

// A brick: it is hit once and it is gone. Only the ball ever touches it — two static bodies never
// produce a contact — so any collision at all is the one that breaks it.
class Brick : public Lion::Component
{
public:
	void OnCollision(Lion::Entity& other) override;
};
