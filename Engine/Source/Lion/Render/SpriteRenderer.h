#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Type/Size.h>

namespace Lion
{
	class Sprite;
	class Texture;

	// Component that draws a Sprite at its owner's Transform every frame.
	//
	// This replaces manual Sprite ownership and drawing inside game objects: attach a
	// SpriteRenderer to any Entity and the Scene takes care of submitting it to the Renderer.
	class SpriteRenderer : public Component
	{
	public:
		LION_API SpriteRenderer(const std::string& filePath = std::string());
		LION_API SpriteRenderer(const Reference<Texture>& texture);
		LION_API ~SpriteRenderer();

		// Returns the pixel size of the underlying texture.
		LION_API Size GetSize() const;

		// Returns the wrapped sprite for advanced tweaking.
		LION_API Sprite& GetSprite() const { return *mSprite; }

		// Source texture path (empty when constructed from an existing texture); used by serialization.
		const std::string& GetTexturePath() const { return mTexturePath; }

		// Replaces the drawn texture by loading a new one from disk (used by the editor Inspector).
		LION_API void SetTexturePath(const std::string& filePath);

		// What this sprite goes on top of. Sprites are drawn low order to high, and sprites sharing one
		// are drawn in the order the Hierarchy lists them — so the default of zero means "wherever my row
		// is", and setting it is overriding the row.
		LION_API int32 GetOrder() const { return mOrder; }
		LION_API void SetOrder(int32 order) { mOrder = order; }

		// Mirroring, read out of the texture rather than taken out of the scale: flipping a sprite should
		// not flip its collider, its children or its maths.
		LION_API bool IsFlippedX() const { return mFlipX; }
		LION_API bool IsFlippedY() const { return mFlipY; }
		LION_API void SetFlipX(bool flip) { mFlipX = flip; }
		LION_API void SetFlipY(bool flip) { mFlipY = flip; }

		void OnRender() override;
		void Serialize(Serializer& serializer) const override;
		void Deserialize(const Serializer& serializer) override;

	private:
		Scope<Sprite> mSprite;
		std::string mTexturePath;
		int32 mOrder = 0;
		bool mFlipX = false;
		bool mFlipY = false;
	};
}
