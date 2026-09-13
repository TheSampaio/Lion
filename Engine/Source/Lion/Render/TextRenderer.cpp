#include "Engine.h"
#include "TextRenderer.h"

#include <Lion/Core/Asset.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Render/BitmapFont.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	TextRenderer::~TextRenderer() = default;

	void TextRenderer::SetText(const std::string& text)
	{
		if (text == mText)
			return;

		mText = text;
		mBuiltText.clear();
	}

	void TextRenderer::SetFontPath(const std::string& path)
	{
		if (path == mFontPath)
			return;

		mFontPath = path;
		mFont.reset();
		mBuiltFontPath.clear();
	}

	void TextRenderer::OnRender()
	{
		if (mText != mBuiltText || mFontPath != mBuiltFontPath)
			Rebuild();

		if (!mFont || mGlyphs.empty())
			return;

		Entity& owner = GetOwner();
		const Vector2 origin = owner.GetWorldPosition();
		const Vector2 ownerScale = owner.GetWorldScale();
		const float32 glyphScale = mSize / std::max(mFont->GetLineHeight(), 1.0f);
		const float32 advance = (mFont->GetAdvance() + mSpacing) * glyphScale * ownerScale.x;
		const float32 lineHeight = mFont->GetLineHeight() * glyphScale * ownerScale.y;

		size_t glyphIndex = 0;
		size_t lineStart = 0;
		float32 lineY = origin.y;

		while (lineStart <= mText.size())
		{
			const size_t lineEnd = mText.find('\n', lineStart);
			const size_t length = (lineEnd == std::string::npos ? mText.size() : lineEnd) - lineStart;
			const float32 width = length > 0 ? advance * static_cast<float32>(length - 1) : 0.0f;
			float32 x = origin.x - (mCentered ? width * 0.5f : 0.0f);

			for (size_t index = lineStart; index < lineStart + length; ++index)
			{
				if (mText[index] != ' ' && glyphIndex < mGlyphs.size())
				{
					Sprite& glyph = *mGlyphs[glyphIndex++];
					glyph.SetOrder(mOrder);
					glyph.SetColor(mColor);
					glyph.Draw(
						Vector(x, lineY, 0.0f),
						Vector(0.0f, 0.0f, owner.GetWorldRotation()),
						Vector(glyphScale * ownerScale.x, glyphScale * ownerScale.y, 1.0f),
						owner.GetId());
				}

				x += advance;
			}

			if (lineEnd == std::string::npos)
				break;

			lineStart = lineEnd + 1;
			lineY -= lineHeight;
		}
	}

	void TextRenderer::Reflect(Reflector& reflector)
	{
		reflector.Field("Text", mText);
		reflector.FieldAsset("Font", mFontPath);
		reflector.Field("Size", mSize);
		reflector.Field("Spacing", mSpacing);
		reflector.Field("Centered", mCentered);
		reflector.Field("Order", mOrder);
		reflector.Field("Color", mColor);
	}

	void TextRenderer::Rebuild()
	{
		mGlyphs.clear();
		mBuiltText = mText;
		mBuiltFontPath = mFontPath;
		mFont = mFontPath.empty() ? nullptr : Asset::LoadFont(mFontPath, mFontPath);

		if (!mFont)
			return;

		mGlyphs.reserve(mText.size());

		for (const char8 character : mText)
		{
			if (character == ' ' || character == '\n')
				continue;

			BitmapGlyph glyphInfo;

			if (!mFont->GetGlyph(static_cast<char8>(std::toupper(static_cast<unsigned char>(character))), glyphInfo))
				continue;

			auto glyph = MakeScope<Sprite>(mFont->GetTexture());
			glyph->SetRegion(glyphInfo.uvMinimum, glyphInfo.uvMaximum, glyphInfo.size);
			mGlyphs.push_back(std::move(glyph));
		}
	}

	LION_REGISTER_COMPONENT(TextRenderer)
}
