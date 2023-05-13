#include "PCH.h"
#include "Sandbox.h"

// Initializes static attributes
bool Sandbox::s_bWireframe = false;

Sandbox::Sandbox()
	: Square(nullptr)
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
	Square = new Quad();

	// Initializes everything in the game
	Square->Start();
}

void Sandbox::Update(float DeltaTime)
{
	// Proccess everything in the game
	if (GetInput()->GetKeyTapped(F2)) { s_bWireframe = !s_bWireframe; GetRenderer()->SetWireframeMode(s_bWireframe); }
	if (GetInput()->GetKeyTapped(F5)) { GetWindow()->Close(true); }

	// Updates everything in the game
	Square->Update(DeltaTime);
}

void Sandbox::Draw()
{
	// Draws everything in the game
	Square->Draw();
}

void Sandbox::Finalize()
{
	// Deletes everything loaded in memory by the game
	delete Square;

	// Just for debbuging
	Debug::Log(Information, "Game Finalized.");
}
