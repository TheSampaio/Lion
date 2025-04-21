#pragma once

#include <Lion/Lion.h>
#include <Lion/Launcher.h>

#include "Source/Layer/CoreLayer.h"
#include "Source/Layer/GameLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		PushLayer(new CoreLayer());
		PushLayer(new GameLayer());
	}
};
