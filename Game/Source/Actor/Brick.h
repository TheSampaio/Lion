#pragma once

#include <Lion/Lion.h>

class Brick : public Lion::Actor
{
public:
	Brick(Lion::Reference<Lion::Texture> texture, Lion::Vector position);

	void OnAwake() override;
	void OnCollision(Lion::Actor& other) override;

	// Whether the brick has been hit and is awaiting removal by the Manager.
	bool IsDestroyed() const { return mDestroyed; }

private:
	Lion::Reference<Lion::Texture> mTexture;
	Lion::Vector mPosition;
	bool mDestroyed = false;
};
