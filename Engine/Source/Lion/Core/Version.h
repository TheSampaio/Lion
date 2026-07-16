#pragma once

namespace Lion
{
	// The engine's version, in one place — the editor's status bar reads it, and so will anything else
	// that has to say which engine this is.
	//
	// It is bumped by hand, because it is a statement about the engine and not a count of builds:
	// the patch for a fix, the minor for something the engine can now do, the major for something it
	// does differently enough to break a game that was written against the old one.
	constexpr const char8* kVersion = "0.12.0";

	// The engine's own icon, shipped beside every executable it builds. A game that never picks one is
	// still a game made with this engine, so this is what it wears.
	constexpr const char8* kEngineIconFile = "Icons/lion-engine-icon.png";

	// The same face in the format Windows wants for a file's icon. A window takes pixels and Explorer takes
	// an .ico, so there are two files and not one.
	constexpr const char8* kEngineIconResource = "Icons/lion-engine-icon.ico";
}
