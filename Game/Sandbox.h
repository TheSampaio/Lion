#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

#include "Source/ExampleLayer.h"
#include "Source/GameplayLayer.h"
#include "Source/WindowLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushLayer(new GameplayLayer());
		PushLayer(new WindowLayer());
	}
};
