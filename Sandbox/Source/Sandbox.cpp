#include "PCH.h"
#include "Sandbox.h"

Sandbox::Sandbox()
	: Quad01(nullptr), Quad02(nullptr), Quad03(nullptr), Quad04(nullptr)
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
	Quad01 = new Mesh(Vertices01, Indices);
	Quad02 = new Mesh(Vertices02, Indices);
	Quad03 = new Mesh(Vertices03, Indices);
	Quad04 = new Mesh(Vertices04, Indices);
}

void Sandbox::Update(float DeltaTime)
{
	// Proccess everything in the game
	if (GetInput()->GetKeyTapped(Escape)) { GetWindow()->Close(true); }
}

void Sandbox::Draw()
{
	// Draw everything in the game
	Quad01->Draw();
	Quad02->Draw();
	Quad03->Draw();
	Quad04->Draw();
}

void Sandbox::Finalize()
{
	// Deletes everything loaded in memory by the game
	delete Quad01;
	delete Quad02;
	delete Quad03;
	delete Quad04;

	// Just for debbuging
	Debug::Log(Information, "Game Finalized.");
}
