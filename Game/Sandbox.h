#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

#include "Source/CoreLayer.h"
#include "Source/GameplayLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		PushLayer(new CoreLayer());
		PushLayer(new GameplayLayer());
	}
};
