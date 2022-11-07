#ifndef GLF3D_GAME_H
#define GLF3D_GAME_H

#include "Window.h"

class Game
{
public:
	Game();
	virtual ~Game();

	virtual void Start() = 0;
	virtual void Update(float& DeltaTime) = 0;
	virtual void Draw() = 0;
	virtual void End() = 0;

	inline void SetWindow(Window*& Window) { s_Window = Window; }

protected:
	inline Window*& GetWindow() const { return s_Window; }

private:
	static Window* s_Window;
};

#endif

