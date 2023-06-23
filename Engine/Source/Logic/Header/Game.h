#pragma once

namespace owl
{
	class OWL_API Game
	{
	public:
		Game() {};

		virtual void OnStart()  = 0;
		virtual void OnUpdate() = 0;
		virtual void OnDraw() = 0;
		virtual void OnFinish() = 0;
	};
}