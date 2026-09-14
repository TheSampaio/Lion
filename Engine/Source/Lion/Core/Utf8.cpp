#include "Engine.h"
#include "Utf8.h"

namespace Lion::Utf8
{
	std::vector<uint32> Decode(const std::string& text)
	{
		std::vector<uint32> codepoints;
		codepoints.reserve(text.size());

		for (size_t index = 0; index < text.size();)
		{
			const uint8 lead = static_cast<uint8>(text[index]);
			uint32 codepoint = 0;
			size_t count = 0;

			if (lead < 0x80) { codepoint = lead; count = 1; }
			else if ((lead & 0xE0) == 0xC0) { codepoint = lead & 0x1F; count = 2; }
			else if ((lead & 0xF0) == 0xE0) { codepoint = lead & 0x0F; count = 3; }
			else if ((lead & 0xF8) == 0xF0) { codepoint = lead & 0x07; count = 4; }

			bool valid = count > 0 && index + count <= text.size();
			for (size_t offset = 1; valid && offset < count; ++offset)
			{
				const uint8 continuation = static_cast<uint8>(text[index + offset]);
				valid = (continuation & 0xC0) == 0x80;
				codepoint = (codepoint << 6) | (continuation & 0x3F);
			}

			const bool overlong = (count == 2 && codepoint < 0x80)
				|| (count == 3 && codepoint < 0x800) || (count == 4 && codepoint < 0x10000);
			if (!valid || overlong || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
			{
				codepoints.push_back(0xFFFD);
				index++;
				continue;
			}

			codepoints.push_back(codepoint);
			index += count;
		}

		return codepoints;
	}

	uint32 ToUpper(uint32 codepoint)
	{
		if (codepoint >= 'a' && codepoint <= 'z')
			return codepoint - 32;
		if ((codepoint >= 0x00E0 && codepoint <= 0x00F6) || (codepoint >= 0x00F8 && codepoint <= 0x00FE))
			return codepoint - 32;
		if (codepoint == 0x00FF)
			return 0x0178;
		if (codepoint >= 0x0430 && codepoint <= 0x044F)
			return codepoint - 32;
		if (codepoint == 0x0451)
			return 0x0401;
		if (codepoint >= 0x03B1 && codepoint <= 0x03C1)
			return codepoint - 32;
		if (codepoint >= 0x03C3 && codepoint <= 0x03CB)
			return codepoint - 32;
		if (codepoint == 0x03C2)
			return 0x03A3;
		return codepoint;
	}
}
