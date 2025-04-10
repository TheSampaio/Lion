#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

#include "Source/GameplayLayer.h"
#include "Source/WindowLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		PushLayer(new WindowLayer());
		PushLayer(new GameplayLayer());
	}
};
