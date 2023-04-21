#include "PCH.h"
#include "Sandbox.h"

Sandbox::Sandbox()
	: Quads(nullptr)
{
	// Setup sandbox's renderer
	GetRenderer()->SetSynchronizationMode(Disabled);

	// Setup sandbox's window
	GetWindow()->SetTitle("Sandbox");
	GetWindow()->SetSize(1360, 768);
	GetWindow()->SetMaximize(false);
	GetWindow()->SetOpenGLVersion(4, 6);
	GetWindow()->SetDisplayMode(Windowed);
	GetWindow()->SetBackgroundColor(230, 245, 250);
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
