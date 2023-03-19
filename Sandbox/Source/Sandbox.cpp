#include "PCH.h"
#include "Sandbox.h"

Sandbox::Sandbox()
{
	// Setup sandbox's window
	GetWindow()->SetTitle("Sandbox");
	GetWindow()->SetSize(1360, 768);
	GetWindow()->SetDisplayMode(Windowed);
	GetWindow()->SetSynchronizationMode(Full);
	GetWindow()->SetBackgroundColor(255, 250, 180);
}

void Sandbox::Start()
{
	GetDebug()->Log(Information, "Game Initialized.");
}

void Sandbox::Update(float DeltaTime)
{
}

void Sandbox::DrawCall()
{
}

void Sandbox::Finalize()
{
	GetDebug()->Log(Information, "Game Finalized.");
}

// ========== Main Function Bellow ========== //

int main()
{
	// Allocates the engine in heap memory
	Application* Engine = new Sandbox;
	Engine->Run();
	delete Engine;
}
