#include "Engine.h"
#include "SpriteRenderer.h"

#include <Lion/Logic/Entity.h>
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

	void SpriteRenderer::SetTexturePath(const std::string& filePath)
	{
		if (filePath == mTexturePath)
			return;

		mSprite = MakeScope<Sprite>(filePath);
		mTexturePath = filePath;
	}

	void SpriteRenderer::OnRender()
	{
		// Draw with the owner's world transform, so children follow their parent.
		Entity& owner = GetOwner();
		mSprite->Draw(
			owner.GetWorldPosition(),
			Vector(0.0f, 0.0f, owner.GetWorldRotation()),
			owner.GetWorldScale(),
			owner.GetId());
	}
}
