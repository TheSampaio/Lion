#include "PCH.h"
#include "Sandbox.h"

// Initializes static attributes
bool Sandbox::bWireframe = false;

Sandbox::Sandbox()
	: Quads(nullptr)
{
	// Set-ups sandbox's renderer
	GetRenderer()->SetOpenGLVersion(4, 6);
	GetRenderer()->SetWireframeMode(false);
	GetRenderer()->SetSynchronizationMode(Disabled);

	// Set-ups sandbox's window
	GetWindow()->SetSize(1360, 768);
	GetWindow()->SetMaximize(false);
	GetWindow()->SetTitle("Sandbox");
	GetWindow()->SetDisplayMode(Windowed);
	GetWindow()->SetBackgroundColour(230, 245, 250);
}

void Sandbox::Start()
{
	// Just for debbuging
	Debug::Log(Information, "Game Initialized.");

	// Load everything that the game will need
	Quads = new Mesh(Vertices, Indices);
}

void Sandbox::Update(float DeltaTime)
{
	// Proccess everything in the game
	if (GetInput()->GetKeyTapped(Escape)) { GetWindow()->Close(true); }
	if (GetInput()->GetKeyTapped(F2)) { bWireframe = !bWireframe; GetRenderer()->SetWireframeMode(bWireframe); }
}

void Sandbox::Draw()
{
	// Draw everything in the game
	Quads->Draw();
}

void Sandbox::Finalize()
{
	// Deletes everything loaded in memory by the game
	delete Quads;

	// Just for debbuging
	Debug::Log(Information, "Game Finalized.");
}
