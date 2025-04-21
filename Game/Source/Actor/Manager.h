#pragma once

#include <Lion/Lion.h>

class Manager : public Lion::Entity
{
public:
	void OnAwake() override;
	void OnRender() override;

private:
	Lion::Reference<Lion::Transform> mTransformBackground;
	Lion::Scope<Lion::Sprite> mSpriteBackground;
};
