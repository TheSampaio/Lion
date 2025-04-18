#pragma once

#include <Lion/Core.h>

class Ball : public Lion::Actor
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void OnRender() override;

private:
	Lion::Scope<Lion::Sprite> mSprite;

	struct
	{
		Lion::float32 x = 400.0f;
		Lion::float32 y = 400.0f;

	} mSpeed;
};
