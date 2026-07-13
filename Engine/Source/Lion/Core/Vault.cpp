#include "Engine.h"
#include "Vault.h"

namespace Lion
{
	namespace
	{
		// 2002, which is a date and not a secret. See the header: this is a lock on a door nobody was going
		// to walk through anyway, and it is spelled out here rather than hidden so that nobody mistakes it
		// for one that is.
		constexpr uint16 kKey = 0x07D2;

		// URL-safe base64: the alphabet everyone knows, with '-' and '_' where it has '+' and '/'. Padding
		// is left off — the length of the data says how much of the last group is real.
		constexpr const char8* kAlphabet =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

		// The key repeats over the content, high byte first. A single byte would leave the file's own
		// structure showing through as a pattern; two is not much better, and is not meant to be.
		byte KeyByte(size_t index)
		{
			return static_cast<byte>((index % 2 == 0) ? (kKey >> 8) : (kKey & 0xFF));
		}

		int32 AlphabetIndex(char8 character)
		{
			for (int32 index = 0; index < 64; ++index)
				if (kAlphabet[index] == character)
					return index;

			return -1;
		}
	}

	std::string Vault::Seal(const std::string& content)
	{
		if (content.empty())
			return content;

		// One pass over the file, once, when it is written. Nothing here happens per frame.
		std::string sealed;
		sealed.reserve((content.size() + 2) / 3 * 4);

		for (size_t index = 0; index < content.size(); index += 3)
		{
			const size_t remaining = content.size() - index;

			uint32 group = 0;

			for (size_t offset = 0; offset < 3; ++offset)
			{
				const byte value = (offset < remaining)
					? static_cast<byte>(static_cast<byte>(content[index + offset]) ^ KeyByte(index + offset))
					: 0;

				group |= static_cast<uint32>(value) << (16 - 8 * offset);
			}

			// Three bytes become four characters; a short last group becomes as many as it has bits for.
			const size_t characters = remaining + 1;

			for (size_t offset = 0; offset < std::min<size_t>(characters, 4); ++offset)
				sealed += kAlphabet[(group >> (18 - 6 * offset)) & 0x3F];
		}

		return sealed;
	}

	std::string Vault::Unseal(const std::string& content)
	{
		// A caller does not have to know which it has: an unsealed file is already what it wanted.
		if (!IsSealed(content))
			return content;

		std::string plain;
		plain.reserve(content.size() / 4 * 3);

		for (size_t index = 0; index < content.size(); index += 4)
		{
			const size_t characters = std::min<size_t>(content.size() - index, 4);

			uint32 group = 0;

			for (size_t offset = 0; offset < characters; ++offset)
				group |= static_cast<uint32>(AlphabetIndex(content[index + offset])) << (18 - 6 * offset);

			for (size_t offset = 0; offset + 1 < characters; ++offset)
			{
				const byte value = static_cast<byte>((group >> (16 - 8 * offset)) & 0xFF);
				plain += static_cast<char8>(value ^ KeyByte(plain.size()));
			}
		}

		return plain;
	}

	bool Vault::IsSealed(const std::string& content)
	{
		if (content.empty())
			return false;

		for (const char8 character : content)
			if (AlphabetIndex(character) < 0)
				return false;

		return true;
	}
}
