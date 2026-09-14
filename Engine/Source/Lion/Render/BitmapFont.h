#pragma once

#include <Lion/Math/Vector.h>
#include <Lion/Type/Size.h>

namespace Lion
{
	class Texture;

	struct BitmapGlyph
	{
		Vector uvMinimum;
		Vector uvMaximum;
		Size size;
	};

	// Fixed-grid bitmap font loaded from a .lnfont descriptor and a shared texture atlas.
	class BitmapFont
	{
	public:
		static LION_API Reference<BitmapFont> Create(const std::string& filePath);

		LION_API const Reference<Texture>& GetTexture() const { return mTexture; }
		LION_API bool GetGlyph(uint32 codepoint, BitmapGlyph& glyph) const;
		LION_API float32 GetAdvance() const { return mAdvance; }
		LION_API float32 GetLineHeight() const { return mLineHeight; }

	private:
		Reference<Texture> mTexture;
		std::vector<uint32> mCodepoints;
		int32 mColumns = 1;
		int32 mRows = 1;
		float32 mGlyphWidth = 1.0f;
		float32 mGlyphHeight = 1.0f;
		float32 mAdvance = 1.0f;
		float32 mLineHeight = 1.0f;
	};
}
