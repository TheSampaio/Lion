#ifndef SANDBOX_GAME_H
#define SANDBOX_GAME_H

#include "Quad.h"

// Main game's class
class Sandbox : public Application
{
public:
    Sandbox();

    // Main methods
    void Start();
    void Update(float DeltaTime);
    void Draw();
    void Finalize();

private:
    // Attributes
    Quad* Square;

	// Static attributes
	static bool s_bWireframe;
};

#endif
