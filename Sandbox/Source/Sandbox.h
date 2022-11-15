#pragma once

#include <GLF3D/Core/Application.h>
#include <GLF3D/Scene/Game.h>

// Main game's class
class MainGame : public Game
{
public:
    void Start();
    void Update(float& DeltaTime);
    void Draw();
    void End();

private:

};
