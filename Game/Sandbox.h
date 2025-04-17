#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

#include "Source/Layer/CoreLayer.h"
#include "Source/Layer/GameplayLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		PushLayer(new CoreLayer());
		PushLayer(new GameplayLayer());
	}
};
