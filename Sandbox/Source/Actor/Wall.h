#pragma once

#include <Lion/Lion.h>

// Invisible static boundary that keeps the ball inside the play area.
class Wall : public Lion::Actor
{
public:
	Wall(Lion::Vector position, Lion::float32 width, Lion::float32 height);

	void OnAwake() override;

private:
	Lion::Vector mPosition;
	Lion::float32 mWidth;
	Lion::float32 mHeight;
};
