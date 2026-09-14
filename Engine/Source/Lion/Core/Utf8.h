#pragma once

namespace Lion::Utf8
{
	// Decodes UTF-8 into Unicode code points. Invalid sequences become U+FFFD and never overrun input.
	LION_API std::vector<uint32> Decode(const std::string& text);

	// Uppercase mapping used by bitmap fonts for the scripts shipped with the engine demo.
	LION_API uint32 ToUpper(uint32 codepoint);
}
