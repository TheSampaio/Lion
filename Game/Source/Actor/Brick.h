#pragma once

#include <Lion/Lion.h>

class Brick : public Lion::Actor
{
public:
	Brick(Lion::Reference<Lion::Texture> texture, const Lion::Vector position);

	void OnRender() override;

private:
	Lion::Scope<Lion::Sprite> mSprite;
};
