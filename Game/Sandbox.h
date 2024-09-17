#pragma once

#include <Lion/Core.h>
#include <Lion/EntryPoint.h>

#include "Source/ExampleLayer.h"

class Sandbox : public Lion::Application
{
public:
	Sandbox()
	{
		Push(new ExampleLayer());
	}
};
