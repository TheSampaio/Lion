#include "Engine.h"
#include "SpriteRenderer.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Serializer.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	SpriteRenderer::SpriteRenderer(const std::string& filePath)
		: mTexturePath(filePath)
	{
		// An empty path leaves the renderer sprite-less (e.g. a default-constructed instance that
		// deserialization is about to point at a real texture), so no bogus load of "" is attempted.
		if (!filePath.empty())
			mSprite = MakeScope<Sprite>(filePath);
	}

	SpriteRenderer::SpriteRenderer(const Reference<Texture>& texture)
		: mSprite(MakeScope<Sprite>(texture))
	{
	}

	SpriteRenderer::~SpriteRenderer() = default;

	Size SpriteRenderer::GetSize() const
	{
		return mSprite ? mSprite->GetSize() : Size(0.0f, 0.0f);
	}

	void SpriteRenderer::SetTexturePath(const std::string& filePath)
	{
		if (filePath == mTexturePath)
			return;

		mSprite = filePath.empty() ? nullptr : MakeScope<Sprite>(filePath);
		mTexturePath = filePath;
	}

	void SpriteRenderer::OnRender()
	{
		if (!mSprite)
			return;

		// How it is drawn travels with the submission: what it goes on top of, and which way round it is
		// read. Set here rather than kept in the sprite, so a texture swapped underneath keeps them.
		mSprite->SetOrder(mOrder);
		mSprite->SetFlip(mFlipX, mFlipY);

		// Draw with the owner's world transform, so children follow their parent.
		Entity& owner = GetOwner();
		mSprite->Draw(
			owner.GetWorldPosition(),
			Vector(0.0f, 0.0f, owner.GetWorldRotation()),
			owner.GetWorldScale(),
			owner.GetId());
	}

	void SpriteRenderer::Serialize(Serializer& serializer) const
	{
		serializer.Write("texture", mTexturePath);
		serializer.Write("order", mOrder);
		serializer.Write("flipX", mFlipX);
		serializer.Write("flipY", mFlipY);
	}

	void SpriteRenderer::Deserialize(const Serializer& serializer)
	{
		SetTexturePath(serializer.ReadString("texture"));
		mOrder = serializer.ReadInt("order", 0);
		mFlipX = serializer.ReadBool("flipX", false);
		mFlipY = serializer.ReadBool("flipY", false);
	}

	LION_REGISTER_COMPONENT(SpriteRenderer)
}
