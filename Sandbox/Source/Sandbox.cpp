#include "PCH.h"
#include "Sandbox.h"

Sandbox::Sandbox()
{
	// Setup sandbox's window
	GetWindow()->SetTitle("Sandbox");
	GetWindow()->SetSize(1366, 768);
	GetWindow()->SetMaximize(false);
	GetWindow()->SetOpenGLVersion(4, 6);
	GetWindow()->SetDisplayMode(Windowed);
	GetWindow()->SetSynchronizationMode(Full);
	GetWindow()->SetBackgroundColor(50, 50, 50);

	ShaderProgram = nullptr;
	VertexArray = nullptr;
	VertexBuffer = nullptr;
	ElementBuffer = nullptr;
}

void Sandbox::Start()
{
	GetDebug()->Log(Information, "Game Initialized.");

	// Create shader program
	ShaderProgram = new Shader("../../GLF3D/Engine/Shaders/DefaultVert.glsl", "../../GLF3D/Engine/Shaders/DefaultFrag.glsl");

	// Create buffers
	VertexArray = new VAO;
	VertexBuffer = new VBO(Vertices);
	ElementBuffer = new EBO(Indices);

	VertexArray->AttribPointer(0, 4, sizeof(Vertex), 0);
	VertexArray->AttribPointer(1, 4, sizeof(Vertex), offsetof(Vertex, Vertex::Color));
}

void Sandbox::Update(float DeltaTime)
{
}

void Sandbox::DrawCall()
{
	ShaderProgram->Activate();
	VertexArray->Bind();

	GetRenderer()->Draw(Indices);
}

void Sandbox::Finalize()
{
	delete ShaderProgram;
	delete VertexArray;
	delete VertexBuffer;
	delete ElementBuffer;

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
