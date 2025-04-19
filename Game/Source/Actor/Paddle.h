#pragma once

#include <Lion/Core.h>

class Paddle : public Lion::Actor
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void OnRender() override;

private:
	Lion::Reference<Lion::Transform> mTransform;
	Lion::Scope<Lion::Sprite> mSprite;

	const Lion::float32 mSpeed = 500.0f;
};
