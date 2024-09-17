#include "ExampleLayer.h"

using namespace Lion;

void ExampleLayer::OnAttach()
{
	Debug& debug = Debug::Get();

	debug.Console(Trace, "Hello World!");
	debug.Console(Information, "Welcome to Lion Engine!");
	debug.Console(Warning, "This is an example layer.");
	debug.Console(Error, "Using the \"OnAttach()\" method.");
}

void ExampleLayer::OnUpdate()
{
	Debug::Get().Console(Trace, "Updating...");
}

void ExampleLayer::OnRender()
{
	Debug::Get().Console(Information, "Drawing...");
}
