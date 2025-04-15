#pragma once

#include <Lion/Core.h>

class GameplayLayer : public Lion::Layer
{
public:
	void OnCreate() override;
	void OnUpdate() override;
	void OnRender() override;

protected:
	void OnEvent(Lion::Event& event) override;
	bool OnEventWindowResize(const Lion::EventWindowResize& event);

private:
	struct
	{
		Lion::float32 x = 400.0f;
		Lion::float32 y = 400.0f;

	} mBallVelocity;

	glm::vec2 mBallPosition = glm::vec2(0.0f, -258.0f);
	glm::vec2 mPlayerPosition = glm::vec2(0.0f, -275.0f);
	Lion::float32 mPlayerVelocity = 500.0f;

	Lion::Reference<Lion::CameraOrthographic> mCamera;
	Lion::Scope<Lion::Sprite> mSpriteBackground;
	Lion::Scope<Lion::Sprite> mSpritePlayer;
	Lion::Scope<Lion::Sprite> mSpriteBall;

	Lion::Reference<Lion::Texture> mTextureBrickRed;
	Lion::Reference<Lion::Texture> mTextureBrickGreen;
	Lion::Reference<Lion::Texture> mTextureBrickBlue;
	Lion::Reference<Lion::Texture> mTextureBrickYellow;
	Lion::Reference<Lion::Texture> mTextureBrickPurple;

	Lion::Reference<Lion::Sprite> mSpriteBrickRed01;
	Lion::Reference<Lion::Sprite> mSpriteBrickGreen01;
	Lion::Reference<Lion::Sprite> mSpriteBrickBlue01;
	Lion::Reference<Lion::Sprite> mSpriteBrickYellow01;
	Lion::Reference<Lion::Sprite> mSpriteBrickPurple01;

	Lion::Reference<Lion::Sprite> mSpriteBrickRed02;
	Lion::Reference<Lion::Sprite> mSpriteBrickGreen02;
	Lion::Reference<Lion::Sprite> mSpriteBrickBlue02;
	Lion::Reference<Lion::Sprite> mSpriteBrickYellow02;
	Lion::Reference<Lion::Sprite> mSpriteBrickPurple02;

	Lion::Reference<Lion::Sprite> mSpriteBrickRed03;
	Lion::Reference<Lion::Sprite> mSpriteBrickGreen03;
	Lion::Reference<Lion::Sprite> mSpriteBrickBlue03;
	Lion::Reference<Lion::Sprite> mSpriteBrickYellow03;
	Lion::Reference<Lion::Sprite> mSpriteBrickPurple03;

	Lion::Reference<Lion::Sprite> mSpriteBrickRed04;
	Lion::Reference<Lion::Sprite> mSpriteBrickGreen04;
	Lion::Reference<Lion::Sprite> mSpriteBrickBlue04;
	Lion::Reference<Lion::Sprite> mSpriteBrickYellow04;
	Lion::Reference<Lion::Sprite> mSpriteBrickPurple04;
	
	Lion::Reference<Lion::Sprite> mSpriteBrickRed05;
	Lion::Reference<Lion::Sprite> mSpriteBrickGreen05;
	Lion::Reference<Lion::Sprite> mSpriteBrickBlue05;
	Lion::Reference<Lion::Sprite> mSpriteBrickYellow05;
	Lion::Reference<Lion::Sprite> mSpriteBrickPurple05;

	Lion::Reference<Lion::Sprite> mSpriteBrickRed06;
	Lion::Reference<Lion::Sprite> mSpriteBrickGreen06;
	Lion::Reference<Lion::Sprite> mSpriteBrickBlue06;
	Lion::Reference<Lion::Sprite> mSpriteBrickYellow06;
	Lion::Reference<Lion::Sprite> mSpriteBrickPurple06;
};
