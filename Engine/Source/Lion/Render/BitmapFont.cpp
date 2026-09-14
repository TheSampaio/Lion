#include "Engine.h"
#include "BitmapFont.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Utf8.h>
#include <Lion/Core/Vault.h>
#include <Lion/Render/Texture.h>

#include <nlohmann/json.hpp>

namespace Lion
{
	Reference<BitmapFont> BitmapFont::Create(const std::string& filePath)
	{
		std::ifstream stream(ResolveResourcePath(filePath), std::ios::binary);

		if (!stream)
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[BitmapFont] Could not open '{}'.", filePath));
			return nullptr;
		}

		std::ostringstream content;
		content << stream.rdbuf();

		try
		{
			const nlohmann::json descriptor = nlohmann::json::parse(Vault::Unseal(content.str()));
			auto font = MakeReference<BitmapFont>();
			font->mCodepoints = Utf8::Decode(descriptor.value("characters", std::string()));
			font->mColumns = std::max(descriptor.value("columns", 1), 1);
			font->mRows = std::max(descriptor.value("rows", 1), 1);
			font->mGlyphWidth = std::max(descriptor.value("glyphWidth", 1.0f), 1.0f);
			font->mGlyphHeight = std::max(descriptor.value("glyphHeight", 1.0f), 1.0f);
			font->mAdvance = std::max(descriptor.value("advance", font->mGlyphWidth), 0.0f);
			font->mLineHeight = std::max(descriptor.value("lineHeight", font->mGlyphHeight), 1.0f);

			const std::string texturePath = descriptor.value("texture", std::string());
			font->mTexture = texturePath.empty() ? nullptr : Asset::LoadTexture(texturePath, texturePath);

			if (!font->mTexture || font->mCodepoints.empty())
			{
				Log::Console(LogLevel::Error,
					LION_FORMAT_TEXT("[BitmapFont] '{}' has no texture or glyphs.", filePath));
				return nullptr;
			}

			return font;
		}
		catch (const std::exception& exception)
		{
			Log::Console(LogLevel::Error,
				LION_FORMAT_TEXT("[BitmapFont] Could not read '{}': {}", filePath, exception.what()));
			return nullptr;
		}
	}

	bool BitmapFont::GetGlyph(uint32 codepoint, BitmapGlyph& glyph) const
	{
		const auto found = std::find(mCodepoints.begin(), mCodepoints.end(), codepoint);

		if (found == mCodepoints.end())
			return false;
		const size_t index = static_cast<size_t>(std::distance(mCodepoints.begin(), found));

		const int32 column = static_cast<int32>(index) % mColumns;
		const int32 row = static_cast<int32>(index) / mColumns;

		if (row >= mRows)
			return false;

		const float32 left = static_cast<float32>(column) / static_cast<float32>(mColumns);
		const float32 right = static_cast<float32>(column + 1) / static_cast<float32>(mColumns);
		const float32 top = 1.0f - static_cast<float32>(row) / static_cast<float32>(mRows);
		const float32 bottom = 1.0f - static_cast<float32>(row + 1) / static_cast<float32>(mRows);

		glyph.uvMinimum = Vector(left, bottom);
		glyph.uvMaximum = Vector(right, top);
		glyph.size = Size(mGlyphWidth, mGlyphHeight);
		return true;
	}
}
