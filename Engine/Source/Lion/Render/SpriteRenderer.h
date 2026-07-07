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
		LION_API SpriteRenderer(const std::string& filePath);
		LION_API SpriteRenderer(const Reference<Texture>& texture);
		LION_API ~SpriteRenderer();

		// Returns the pixel size of the underlying texture.
		LION_API Size GetSize() const;

		// Returns the wrapped sprite for advanced tweaking.
		LION_API Sprite& GetSprite() const { return *mSprite; }

		// Source texture path (empty when constructed from an existing texture); used by serialization.
		const std::string& GetTexturePath() const { return mTexturePath; }

		void OnRender() override;

	private:
		Scope<Sprite> mSprite;
		std::string mTexturePath;
	};
}
