#pragma once

#include <GLF3D.h>

#include "Core/Application.h"
#include "Core/Debugger.h"
#include "Scene/Game.h"

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
