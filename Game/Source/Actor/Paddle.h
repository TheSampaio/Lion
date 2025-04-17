#pragma once

#include <Lion/Core.h>

class Paddle : public Lion::Actor
{
public:
	void OnAwake() override;
	void OnRender() override;

private:
	Lion::Scope<Lion::Sprite> mSprite;
};
