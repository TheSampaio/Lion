#include "Manager.h"
#include "Brick.h"

using namespace Lion;

void Manager::OnAwake()
{
	mTransformBackground = MakeReference<Transform>();
	mTransformBackground->SetPosition(Vector(0.0, 0.0f, Depth::Back));

	mSpriteBackground = MakeScope<Sprite>("Resource/Sprite/Brickout/background.jpg");

	const float32 spacingX = 80.0f;
	const float32 spacingY = 40.0f;
	const int32 colCount = 8;
	const int32 rowCount = 5; // Max rows are 5, don't increase!

	const float32 positionOffset = 10.0f;
	const float32 positionX = -(colCount * (spacingX - positionOffset)) / 2.0f; // Center all bricks
	const float32 positionY = 200.0f;

	// World coordinate system: (0,0) is the center of the screen
	for (int32 row = 0; row < rowCount; row++)
	{
		for (int32 col = 0; col < colCount; col++)
		{
			Reference<Texture> texture;

			switch (row)
			{
				case 0: texture = Asset::LoadTexture("brick_red",    "Resource/Sprite/Brickout/tile-3.png"); break;
				case 1: texture = Asset::LoadTexture("brick_green",  "Resource/Sprite/Brickout/tile-1.png"); break;
				case 2: texture = Asset::LoadTexture("brick_blue",   "Resource/Sprite/Brickout/tile-2.png"); break;
				case 3: texture = Asset::LoadTexture("brick_yellow", "Resource/Sprite/Brickout/tile-5.png"); break;
				case 4: texture = Asset::LoadTexture("brick_purple", "Resource/Sprite/Brickout/tile-4.png"); break;
			}

			float32 x = positionX + col * spacingX;
			float32 y = positionY - row * spacingY;

			GetScene()->Add(MakeReference<Brick>(texture, Vector(x, y, Depth::Middle)));
		}
	}
}

void Manager::OnRender()
{
	mSpriteBackground->Draw(mTransformBackground);
}
