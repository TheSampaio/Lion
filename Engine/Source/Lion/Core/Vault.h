#pragma once

namespace Lion
{
	// Sealing an asset.
	//
	// What a project works on has to be a file a person can open, diff and edit; what a game ships should
	// not be one a player can edit. Those are the same file at two different moments, so it is sealed on
	// its way out and never on its way in — the project keeps plain text, and the build hands over
	// something nobody opens by accident.
	//
	// It is obfuscation, not encryption, and it does not pretend otherwise: the key is in the binary, and
	// a key you ship is a key you gave away. It costs a determined reader an afternoon and it costs
	// everyone else the idea of editing the file in Notepad, which is the whole of what it is for.
	//
	// The bytes are XOR'd against a two-byte key and then written in base64 with the URL-safe alphabet —
	// '-' and '_' where the usual one has '+' and '/'. The result is text, so it goes wherever text goes
	// and no tool along the way decides to reinterpret a byte of it. It travels in lines of 76 characters,
	// the width base64 has worn since MIME, so a sealed file still looks like a file and not like one
	// endless line; unsealing takes the breaks back out before it reads a thing.
	class Vault
	{
	public:
		// Seals plain content, and opens sealed content. Unsealing something that was never sealed gives it
		// back unchanged, so a caller that does not know which it has can simply ask.
		static LION_API std::string Seal(const std::string& content);
		static LION_API std::string Unseal(const std::string& content);

		// Whether the content is sealed. Sealed content is base64 and nothing else, so the question answers
		// itself: a shader has a '#' in it, a scene has a '{', and neither letter is in the alphabet.
		static LION_API bool IsSealed(const std::string& content);
	};
}
