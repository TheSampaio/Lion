#pragma once

#include <Lion/Lion.h>

class Brick : public Lion::Actor
{
public:
	Brick(Lion::Reference<Lion::Texture> texture, Lion::Vector position);

	void OnAwake() override;
	void OnCollision(Lion::Actor& other) override;

private:
	Lion::Reference<Lion::Texture> mTexture;
	Lion::Vector mPosition;
};
