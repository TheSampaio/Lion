#ifndef SANDBOX_GAME_H
#define SANDBOX_GAME_H

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
    // Create static meshes
    Mesh* Quad01;
    Mesh* Quad02;
    Mesh* Quad03;
    Mesh* Quad04;
};

#endif
