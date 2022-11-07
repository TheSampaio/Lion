#ifndef GLF3D_APPLICATION_H
#define GLF3D_APPLICATION_H

#include "Game.h"
#include "Window.h"

class Application
{
public:
    Application();
    ~Application();

    // Main methods
    bool Start(Game* Level);

    // Get methods
    inline Window*& GetWindow() const { return s_Window; }

private:
    bool Run();

    static Game* s_Game;
    static Window* s_Window;
};

#endif
