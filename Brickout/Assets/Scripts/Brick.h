#pragma once

#include <Lion/Lion.h>

// A brick: it is hit once and becomes inactive. It stays in the authored scene so the round controller
// can restore it without carrying a second, code-only description of the level.
class Brick : public Lion::Component
{
public:
	void OnCollision(Lion::Entity& other) override;
};
