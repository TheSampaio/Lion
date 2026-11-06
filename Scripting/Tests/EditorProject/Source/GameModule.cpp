#include <Lion/Lion.h>

// Test fixture matching the generated project bootstrap. Gameplay stays in EditorProbe.cs.
extern "C" __declspec(dllexport) Lion::Application* LionCreateApplication()
{
	return new Lion::Application();
}
