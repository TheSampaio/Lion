#pragma once

#include <Lion/Core.h>

class Brick : public Lion::Actor
{
public:
	Brick(Lion::Reference<Lion::Texture> texture, const glm::vec3 position);

	void OnRender() override;

private:
	glm::vec3 mPosition;
	Lion::Scope<Lion::Sprite> mSprite;
};
