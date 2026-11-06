#include <Lion/Base/Platform.h>
#include <Lion/Runtime/ScenePlayer.h>

// The packaged scene bootstrap owns no gameplay and is shared by C#-only projects.
extern "C" __declspec(dllexport) Lion::Application* LionCreateApplication()
{
	return Lion::CreateScenePlayer();
}
