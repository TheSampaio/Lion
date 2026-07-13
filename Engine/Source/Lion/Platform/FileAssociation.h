#pragma once

namespace Lion
{
	// Teaching Windows what a file of ours is.
	//
	// A registration says three things: what a file of this extension is called, what it looks like in
	// Explorer, and which program opens it. It is written under HKCU\Software\Classes — the user's own view
	// of the registry — so it asks for no administrator and touches nobody else's machine. That is where
	// every editor that does this writes it, and it is why the first run does not put up a prompt.
	//
	// What it cannot do is make the program the *default* handler behind the user's back: Windows guards
	// that choice, and an application that could take it would be an application that could take it from
	// you. What it can do is claim an extension nobody has claimed — where Explorer then shows the icon and
	// a double-click opens the editor — and stand in "Open with" where somebody has. That is the whole of
	// what is on offer.
	class FileAssociation
	{
	public:
		// Registers 'extension' (with the dot) to a program identifier of its own.
		//
		// Returns whether anything was written. It writes on the first run and after the editor moves, and
		// on every other run it finds what it was going to write already there and leaves the registry
		// alone — a program that rewrites the user's registry every time it starts is a program that has
		// mistaken "idempotent" for "free".
		static LION_API bool Register(
			const std::string& extension,
			const std::string& programId,
			const std::string& description,
			const std::string& iconPath,
			const std::string& applicationPath);
	};
}
