#include "Engine.h"
#include "SpriteRenderer.h"

#include <Lion/Render/Sprite.h>

namespace Lion
{
	SpriteRenderer::SpriteRenderer(const std::string& filePath)
		: mSprite(MakeScope<Sprite>(filePath)), mTexturePath(filePath)
	{
	}

	SpriteRenderer::SpriteRenderer(const Reference<Texture>& texture)
		: mSprite(MakeScope<Sprite>(texture))
	{
	}

	SpriteRenderer::~SpriteRenderer() = default;

	Size SpriteRenderer::GetSize() const
	{
		return mSprite->GetSize();
	}

	void SpriteRenderer::OnRender()
	{
		mSprite->Draw(GetTransform());
	}
}
