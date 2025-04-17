#pragma once

#include <Lion/Core.h>

class GameManager : public Lion::Entity
{
public:
	void OnAwake() override;
	void OnRender() override;

private:
	Lion::Scope<Lion::Sprite> mSpriteBackground;
};
