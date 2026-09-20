#include "Engine.h"
#include "TextRenderer.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Utf8.h>
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

	void TextRenderer::CopyStyleTo(TextRenderer& target) const
	{
		target.SetFontPath(mFontPath);
		target.SetSize(mSize);
		target.SetSpacing(mSpacing);
		target.SetCentered(mCentered);
		target.SetOrder(mOrder);
		target.SetOffset(mOffset);
		target.SetColor(mColor);
	}

	void TextRenderer::OnRender()
	{
		if (mText != mBuiltText || mFontPath != mBuiltFontPath)
			Rebuild();

		if (!mFont || mGlyphs.empty())
			return;

		Entity& owner = GetOwner();
		const Vector2 position = owner.GetWorldPosition();
		const Vector2 ownerScale = owner.GetWorldScale();
		const float32 radians = owner.GetWorldRotation() * 3.14159265359f / 180.0f;
		const float32 cosine = std::cos(radians);
		const float32 sine = std::sin(radians);
		const Vector2 scaledOffset(mOffset.x * ownerScale.x, mOffset.y * ownerScale.y);
		const Vector2 origin(
			position.x + scaledOffset.x * cosine - scaledOffset.y * sine,
			position.y + scaledOffset.x * sine + scaledOffset.y * cosine);
		const float32 glyphScale = mSize / std::max(mFont->GetLineHeight(), 1.0f);
		const float32 advance = (mFont->GetAdvance() + mSpacing) * glyphScale * ownerScale.x;
		const float32 lineHeight = mFont->GetLineHeight() * glyphScale * ownerScale.y;

		size_t glyphIndex = 0;
		size_t lineStart = 0;
		float32 lineY = origin.y;

		while (lineStart <= mCodepoints.size())
		{
			const auto lineFound = std::find(mCodepoints.begin() + lineStart, mCodepoints.end(), '\n');
			const size_t lineEnd = static_cast<size_t>(std::distance(mCodepoints.begin(), lineFound));
			const size_t length = lineEnd - lineStart;
			const float32 width = length > 0 ? advance * static_cast<float32>(length - 1) : 0.0f;
			float32 x = origin.x - (mCentered ? width * 0.5f : 0.0f);

			for (size_t index = lineStart; index < lineStart + length; ++index)
			{
				if (mCodepoints[index] != ' ' && glyphIndex < mGlyphs.size())
				{
					Sprite* glyph = mGlyphs[glyphIndex++].get();
					if (!glyph)
					{
						x += advance;
						continue;
					}
					glyph->SetOrder(mOrder);
					glyph->SetColor(mColor);
					glyph->Draw(
						Vector(x, lineY, 0.0f),
						Vector(0.0f, 0.0f, owner.GetWorldRotation()),
						Vector(glyphScale * ownerScale.x, glyphScale * ownerScale.y, 1.0f),
						owner.GetId());
				}

				x += advance;
			}

			if (lineFound == mCodepoints.end())
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
		reflector.Field("Offset", mOffset);
		reflector.Field("Color", mColor);
	}

	void TextRenderer::Rebuild()
	{
		mGlyphs.clear();
		mCodepoints = Utf8::Decode(mText);
		mBuiltText = mText;
		mBuiltFontPath = mFontPath;
		mFont = mFontPath.empty() ? nullptr : Asset::LoadFont(mFontPath, mFontPath);

		if (!mFont)
			return;

		mGlyphs.reserve(mCodepoints.size());

		for (const uint32 codepoint : mCodepoints)
		{
			if (codepoint == ' ' || codepoint == '\n')
				continue;

			BitmapGlyph glyphInfo;

			if (!mFont->GetGlyph(Utf8::ToUpper(codepoint), glyphInfo))
			{
				mGlyphs.push_back(nullptr);
				continue;
			}

			auto glyph = MakeScope<Sprite>(mFont->GetTexture());
			glyph->SetRegion(glyphInfo.uvMinimum, glyphInfo.uvMaximum, glyphInfo.size);
			mGlyphs.push_back(std::move(glyph));
		}
	}

	LION_REGISTER_COMPONENT(TextRenderer)
}
