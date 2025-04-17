#pragma once

#include <Lion/Core.h>

class Ball : public Lion::Actor
{
public:
	void OnAwake() override;
	void OnRender() override;

private:
	Lion::Scope<Lion::Sprite> mSprite;
};
