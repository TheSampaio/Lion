#pragma once

#include <Lion/Lion.h>

#include "Source/Layer/ProjectManagerLayer.h"

// The application the executable becomes when it is started with no project to open: the window that
// lists them. Picking one starts the editor as its own process (see ProjectManagerLayer::OpenInEditor),
// so this application never turns into the editor — it ends where the editor begins.
class ProjectManager : public Lion::Application
{
public:
	ProjectManager()
	{
		PushLayer(new ProjectManagerLayer());
	}
};
