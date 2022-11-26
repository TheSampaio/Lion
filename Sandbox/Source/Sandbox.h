#ifndef SANDBOX_GAME_H
#define SANDBOX_GAME_H

#include <GLF3D/Core/Application.h>
#include <GLF3D/Scene/Game.h>

// Main game's class
class Sandbox : public Game
{
public:
    void Start();
    void Update(float& DeltaTime);
    void Draw();
    void End();

private:

};

#endif
