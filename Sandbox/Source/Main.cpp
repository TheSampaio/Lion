#include "PCH.h"
#include "Sandbox.h"

int main()
{
	// Allocates the engine in heap memory
	Application* Engine = new Sandbox;
	Engine->Run();
	delete Engine;
}
