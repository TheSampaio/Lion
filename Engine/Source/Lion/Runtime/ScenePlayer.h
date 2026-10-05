#pragma once

namespace Lion
{
	class Application;

	// Creates the standard scene-authored player. Config/Player.lnplayer selects its entry scene and
	// optional managed catalog; the exporter writes that resource-relative configuration.
	LION_API Application* CreateScenePlayer();
}
