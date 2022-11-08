#include "PCH.h"
#include "Application.h"

#include "Game.h"

// Main game's class
class SuperGame : public Game
{
public:
    void Start();
    void Update(float& DeltaTime);
    void Draw();
    void End();

private:

};

void SuperGame::Start()
{
    Debug::Log::Info("The game was initialized.");
}

void SuperGame::Update(float& DeltaTime)
{
}

void SuperGame::Draw()
{
}

void SuperGame::End()
{
    Debug::Log::Info("The game was finalized.");
}

// ========== Entry Point ========== //
int main()
{    
    Application Application;

    Application.GetWindow()->SetSize(1280, 720);
    Application.GetWindow()->SetTitle("Super Game");
    Application.GetWindow()->SetBackgroundColor(0.80f, 0.85f, 0.95f);

    return Application.Start(new SuperGame);
}
