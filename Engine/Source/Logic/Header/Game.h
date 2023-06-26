#pragma once

namespace owl
{
	class OWL_API Game
	{
	public:
		Game() {};

		// === MAIN methods ======

		// It is called before the first frame update
		virtual void OnStart()  = 0;

		// It is called once every frame to update game entities
		virtual void OnUpdate() = 0;

		// It is called once every frame to draw game entities
		virtual void OnDraw() = 0;

		// It is called before the game close outside the game loop
		virtual void OnFinish() = 0;

		// It is called every time the game is paused or loses focus
		virtual void OnPause();
	};
}