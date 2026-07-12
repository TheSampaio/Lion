#pragma once

namespace Lion
{
	// The configuration this was compiled in: "Debug", "Release" or "Shipping".
	//
	// Shown in the window title, and used by the editor to build the game module against the same one
	// — a module compiled for another configuration links a different runtime and will not load.
	LION_API const char8* BuildConfiguration();
}
