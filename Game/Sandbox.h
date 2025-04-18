#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

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
