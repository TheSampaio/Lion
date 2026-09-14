#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>

namespace Lion
{
	class BitmapFont;
	class Sprite;

	// Draws plain text from a bitmap-font atlas. Every glyph shares one texture and remains part of the
	// normal sprite batch, so game UI does not need a separate rendering backend.
	class TextRenderer : public Component
	{
	public:
		LION_API TextRenderer() = default;
		LION_API ~TextRenderer();

		LION_API const std::string& GetText() const { return mText; }
		LION_API void SetText(const std::string& text);
		LION_API void SetFontPath(const std::string& path);
		LION_API void SetColor(const Vector& color) { mColor = color; }

		void OnRender() override;
		void Reflect(Reflector& reflector) override;

	private:
		std::string mText = "TEXT";
		std::string mFontPath;
		float32 mSize = 32.0f;
		float32 mSpacing = 0.0f;
		bool mCentered = true;
		int32 mOrder = 100;
		Vector mColor = Vector(1.0f);

		Reference<BitmapFont> mFont;
		std::vector<Scope<Sprite>> mGlyphs;
		std::vector<uint32> mCodepoints;
		std::string mBuiltText;
		std::string mBuiltFontPath;

		void Rebuild();
	};
}
