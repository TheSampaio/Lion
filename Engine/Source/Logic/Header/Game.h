#pragma once

namespace owl
{
	class Game
	{
	public:
		Game() {};

		// === MAIN methods ======

		// It is called before the first frame update
		virtual void OWL_API OnStart()  = 0;

		// It is called once every frame to update game entities
		virtual void OWL_API OnUpdate() = 0;

		// It is called once every frame to draw game entities
		virtual void OWL_API OnDraw() = 0;

		// It is called before the game close outside the game loop
		virtual void OWL_API OnFinish() = 0;

		// It is called every time the game is paused or loses focus
		virtual void OWL_API OnPause();
	};
}