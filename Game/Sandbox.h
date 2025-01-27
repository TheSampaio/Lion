#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

#include "Source/ExampleLayer.h"
#include "Source/GameplayLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushLayer(new GameplayLayer());
	}
};
