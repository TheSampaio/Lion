#pragma once

#include <Lion/Lion.h>
#include <Lion/Launcher.h>

#include "Source/Layer/EditorLayer.h"

// The Lion editor application: a separate tool that links the engine and hosts the editor UI.
class Editor : public Lion::Application
{
public:
	Editor()
	{
		PushLayer(new EditorLayer());
	}
};
